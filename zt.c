#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <termios.h>
#include <pwd.h>
#include <errno.h>
#include <locale.h>

#if defined(__linux)
#include <pty.h>
#elif defined(__APPLE__)
#include <util.h>
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <libutil.h>
#endif

#include "zt.h"
#include "ctrl.h"
#include "config.h"

struct ZT zt;
pid_t pid;

int
io_wait(int *r, int nr, int *w, int nw, long nano) {
    fd_set rfd, wfd, *prfd=NULL, *pwfd=NULL;
    int  m=-1, ret=0, i;
    struct timespec tv, *ptv;

    if (nr <= 0 && nw <= 0)
        return 0;

    ptv = NULL;
    if (nano > 0) {
        tv.tv_sec = nano / NANOSEC;
        tv.tv_nsec = nano % NANOSEC;
        ptv = &tv;
    } 
    if (nr > 0)
        prfd = &rfd;
    if (nw > 0)
        pwfd = &wfd;

    FD_ZERO(&rfd);
    FD_ZERO(&wfd);

    for (i = 0; i < nr; i++) {
        if (r[i] > m)
            m = r[i];
        FD_SET(r[i], &rfd);
    }

    for (i = 0; i < nw; i++) {
        if (w[i] > m)
            m = w[i];
        FD_SET(w[i], &wfd);
    }

    ret = pselect(m+1, prfd, pwfd, NULL, ptv, NULL);
    if (ret < 0) {  //error
        ASSERT(errno == EINTR, "select failed: %s", strerror(errno));
        return 0;
    }

    if (ret == 0) // timeout
        return 0;

    for (i = 0; i < nr; i++)
        if (FD_ISSET(r[i], prfd))
            return -(i+1);

    for (i = 0; i < nw; i++)
        if (FD_ISSET(w[i], pwfd))
            return i+1;

    return 0;

}

void
cleanup(void) {
    lclean();
    xclean();
    close(zt.tty);
    printf("exit\n");
    _exit(0);
}

void
tdrawed(void) {
    ldirty_reset();
}

int
tread(int wait) {
    static unsigned char buf[BUFSIZ];
    static int n=0;
    int ret, m;

    ASSERT(n >= 0, "");
    if (n == BUFSIZ) {
        parse(buf, n, 1);
        n = 0;
    }

    if (wait > 0 && io_wait(&zt.tty, 1, NULL, 0, wait) != -1)
        return 1;

    ret = read(zt.tty, buf+n, sizeof(buf)-n);
    if (ret < 0) {
        printf("failed to read tty: %s\n", strerror(errno));
        return 0;
    }

    n += ret;
    m = parse(buf, n, 0);
    n -= m;
    if (n>0)
        memmove(buf, buf+m, n);
    return 0;
}

void
twrite(char *s, int n) {
    int ret, ntry=0, wait=100 * MICROSEC;

    for (;;) {
        if (n<=0)
            break;
        ntry++;
        ASSERT(ntry <= 100, "can't write tty");

        if (io_wait(NULL, 0, &zt.tty, 1, wait) != 1)
            continue;
        ret = write(zt.tty, s, n);
        if (ret < 0)  {
            printf("failed to read tty: %s\n", strerror(errno));
            return;
        }

        if (ret == n || !tread(wait))
            ntry = 0;

        n -= ret;
        s += ret;
    }
}

void
sigchld(int a UNUSED) {
    int stat;
    pid_t p;

    p = waitpid(pid, &stat, WNOHANG);
    ASSERT(p >= 0, "failed to wait pid %d: %s",
        pid, strerror(errno));
    if (pid != p)
        return;
    cleanup();
}

void
tresize(void) {
    struct winsize ws;
    int ret;

    ws.ws_row = zt.row;
    ws.ws_col = zt.col;
    ws.ws_xpixel = zt.width;
    ws.ws_ypixel = zt.height;
    ret = ioctl(zt.tty, TIOCSWINSZ, &ws);
    ASSERT(ret >= 0, "failed to set tty size: %s", strerror(errno));
}

void
tinit(void) {
    char *sh, *args[2];
    struct passwd *pw;
    int ret, slave;

    ret = openpty(&zt.tty, &slave, NULL, NULL, NULL);
    ASSERT(ret>=0, "openpty failed: %s", strerror(errno));

    sh = getenv("SHELL");
    ASSERT(sh != NULL, "");

    pw = getpwuid(getuid());
    ASSERT(pw != NULL, "");

    pid = fork();
    ASSERT(pid != -1, "fork failed: %s", strerror(errno));

    if (pid) {
        close(slave);
        signal(SIGCHLD, sigchld);
        return;
    } 

    setsid();
    dup2(slave, 0);
    dup2(slave, 1);
    dup2(slave, 2);
    ret = ioctl(slave, TIOCSCTTY, NULL);
    ASSERT(ret >= 0, "ioctl TIOCSCTTY failed: %s", strerror(errno));
    close(slave);
    close(zt.tty);

    args[0] = sh;
    args[1] = NULL;
    setenv("SHELL", sh, 1);
    setenv("TERM", TERM, 1);
    setenv("HOME", pw->pw_dir, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("LONGNAME", pw->pw_name, 1);
    execvp(sh, args);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
    _exit(1);
}

int
main(void) {
    int fd[2], ret;
    long now, last, latency, timeout;

    zt.row = MAX(ROW, 8);
    zt.col = MAX(COL, 8);
    setlocale(LC_CTYPE, "");
    MODE_RESET();

    tinit();
    xinit();
    linit();
    tresize();

    last = get_time();
    fd[0] = zt.xfd;
    fd[1] = zt.tty;
    latency = LATENCY * MICROSEC;
    timeout = -1;
    for (;;){
        ret = io_wait(fd, 2, NULL, 0, timeout);

        if (ret != -1 && ret != -2)
            ASSERT(timeout > 0, "can't be");

        if (ret == -1 && xevent())
            break;

        if (ret == -2)
             ASSERT(tread(-1) == 0, "can't be");

        now = get_time();
        latency -= now-last;
        last = now;
	    //printf("%ld\n", latency);

	    if (latency >= 0) {
            timeout = latency;
            continue;
	    } 

        latency = LATENCY * MICROSEC;
        timeout = -1;
        xdraw();
        tdrawed();

    }

    cleanup();
    return 0;
}
