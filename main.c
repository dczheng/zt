#include <pwd.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#if defined(__linux)
#include <pty.h>
#elif defined(__APPLE__)
#include <util.h>
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <libutil.h>
#endif

#include "zt.h"
#include "code.h"

#define LATENCY 1

struct zt_t zt = {0};
int tty = 0;
pid_t pid;

void ldirty_reset(void);
void cfree(void);
void cinit();
int ctrl(uint8_t*, int, int);
void xinit(void);
void xfree(void);
int xevent(void);
void xdraw(void);

int
io_wait(int *r, int nr, int *w, int nw, long nano) {
    fd_set rfd, wfd, *prfd = NULL, *pwfd = NULL;
    int m = -1, ret = 0, i;
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
clean(void) {
    xfree();
    cfree();
    close(tty);
    if (zt.log >= 0)
        close(zt.log);
    _exit(0);
}

int
tread(int wait) {
    static uint8_t buf[BUFSIZ];
    static int n = 0;
    int ret, m;

    ASSERT(n >= 0 && n < (int)sizeof(buf), "");

    if (wait > 0 && io_wait(&tty, 1, NULL, 0, wait) != -1)
        return 1;

    ret = read(tty, buf+n, sizeof(buf)-n);
    if (ret < 0) {
        LOG("failed to read tty: %s\n", strerror(errno));
        return 0;
    }

    n += ret;
    m = ctrl(buf, n, 0);
    n -= m;
    if (n>0)
        memmove(buf, buf+m, n);
    return 0;
}

void
twrite(char *s, int n) {
    int ret, ntry = 0, wait = 100 * MICROSEC;

    for (;;) {
        if (n <= 0) break;

        ntry++;
        ASSERT(ntry <= 100, "can't write tty");

        if (io_wait(NULL, 0, &tty, 1, wait) != 1)
            continue;
        ret = write(tty, s, n);
        if (ret < 0)  {
            LOG("failed to read tty: %s\n", strerror(errno));
            return;
        }
        n -= ret;
        s += ret;
    }
}

void
sigchld(int a __unused) {
    int stat;
    pid_t p;

    p = waitpid(pid, &stat, WNOHANG);
    ASSERT(p >= 0, "failed to wait pid %d: %s",
        pid, strerror(errno));
    if (pid != p)
        return;
    clean();
}

void
tresize(void) {
    struct winsize ws;
    int ret;

    ws.ws_row = zt.row;
    ws.ws_col = zt.col;
    ws.ws_xpixel = zt.width;
    ws.ws_ypixel = zt.height;
    ret = ioctl(tty, TIOCSWINSZ, &ws);
    ASSERT(ret >= 0, "failed to set tty size: %s", strerror(errno));
}

void
tinit(void) {
    char *sh, *args[2];
    struct passwd *pw;
    int ret, slave;

    ret = openpty(&tty, &slave, NULL, NULL, NULL);
    ASSERT(ret >= 0, "openpty failed: %s", strerror(errno));

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
    if (slave > 2)
        close(slave);
    close(tty);

    args[0] = sh;
    args[1] = NULL;

    unsetenv("COLUMNS");
    unsetenv("LINES");
    unsetenv("TERMCAP");

    setenv("SHELL", sh, 1);
    setenv("TERM", TERM, 1);
    setenv("HOME", pw->pw_dir, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("LONGNAME", pw->pw_name, 1);

    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGALRM, SIG_DFL);

    execvp(sh, args);
    _exit(1);
}

int
main(int argc, char **argv) {
    int fd[2], ret, i;
    long now, last, latency, timeout;
    struct option opts [] = {
        {"log",             required_argument, NULL, 1},
        {"font-size",       required_argument, NULL, 2},
        {"debug-ctrl",      no_argument,       NULL, 3},
        {"debug-term",      required_argument, NULL, 4},
        {"debug-retry",     no_argument,       NULL, 5},
        {"debug-x",         no_argument,       NULL, 6},
        {0, 0, 0, 0}
    };

    zt.fontsize = 1;
    zt.log = -1;
    while ((i = getopt_long_only(argc, argv, "", opts, NULL)) != -1) {
        switch(i) {
        case 1:
            if ((zt.log = open(optarg,
                O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                LOG("failed to open log: %s\n", optarg);
                break;
            }
            dup2(zt.log, 1);
            dup2(zt.log, 2);
            break;
        case 2:
            if (stod(&zt.fontsize, optarg))
                zt.fontsize = 1;
            break;
        case 3:
            zt.debug.ctrl = 1;
            break;
        case 4:
            if (stoi(&zt.debug.term, optarg))
                zt.debug.term = 0;
            break;
        case 5:
            zt.debug.retry = 1;
            break;
        case 6:
            zt.debug.x = 1;
            break;
        }
    }

    cinit();
    xinit();
    tinit();
    tresize();

    last = get_time();
    fd[0] = zt.xfd;
    fd[1] = tty;
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
        ldirty_reset();

    }

    clean();
    return 0;
}
