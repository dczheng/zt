#include <pwd.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
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

struct zt_t zt = {0};
int tty = 0, running;
pid_t tty_pid;

void tinit();
void tfree(void);
int twrite(uint8_t*, int);
void ldirty_reset(void);

int xinit(void);
void xfree(void);
int xevent(void);
void xdraw(void);

void
tty_read(void) {
    static uint8_t buf[BUFSIZ];
    static int n = 0;
    int ret, m;

    ASSERT(n >= 0 && n < (int)sizeof(buf));
    if ((ret = read(tty, buf+n, sizeof(buf)-1)) < 0) {
        if (errno != EIO)
            LOGERR("failed to read tty: %s\n", strerror(errno));
        return;
    }

    n += ret;
    m = twrite(buf, n);
    n -= m;
    if (n > 0)
        memmove(buf, buf+m, n);
}

void
tty_write(char *s, int n) {
    int ret;
    fd_set fds;
    struct timespec tv;

    tv = to_timespec(SECOND);
    for (;;) {
        if (n <= 0) break;

        FD_ZERO(&fds);
        FD_SET(tty, &fds);
        ASSERT(pselect(tty+1, NULL, &fds, NULL, &tv, NULL) > 0);

        if ((ret = write(tty, s, n)) < 0)  {
            if (errno != EIO)
                LOGERR("failed to read tty: %s\n", strerror(errno));
            return;
        }
        n -= ret;
        s += ret;
    }
}

void
tty_resize(void) {
    struct winsize ws;
    int ret;

    ws.ws_row = zt.row;
    ws.ws_col = zt.col;
    ws.ws_xpixel = zt.width;
    ws.ws_ypixel = zt.height;
    ASSERT((ret = ioctl(tty, TIOCSWINSZ, &ws)) >= 0);
}

void
sigchld(int a __unused) {
    int stat;
    pid_t p;

    ASSERT((p = waitpid(tty_pid, &stat, WNOHANG)) >= 0);
    if (tty_pid != p)
        return;
    running = 0;
}

void
tty_init(void) {
    char *sh, *args[2];
    struct passwd *pw;
    int ret, slave;

    ASSERT((ret = openpty(&tty, &slave, NULL, NULL, NULL)) >= 0);
    ASSERT(pw = getpwuid(getuid()));

    if (!(sh = getenv("SHELL"))) {
        if (pw->pw_shell[0])
            sh = pw->pw_shell;
        else
            sh = "/bin/sh";
    }

    ASSERT((tty_pid = fork()) != -1);
    if (tty_pid) {
        close(slave);
        signal(SIGCHLD, sigchld);
        return;
    }
    signal(SIGCHLD, sigchld);

    setsid();
    close(tty);
    dup2(slave, 0);
    dup2(slave, 1);
    dup2(slave, 2);
    ASSERT((ret = ioctl(slave, TIOCSCTTY, NULL)) >= 0);
    if (slave > 2)
        close(slave);

    setenv("SHELL", sh, 1);
    setenv("TERM", zt.term, 1);
    setenv("HOME", pw->pw_dir, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("LONGNAME", pw->pw_name, 1);

    args[0] = sh;
    args[1] = NULL;
    execvp(sh, args);
    _exit(1);
}

int
main(int argc, char **argv) {
    int ret, i, xfd;
    struct timespec tv;
    fd_set fds;
    struct option opts[] = {
        {"font-size",       required_argument, NULL, 1},
        {"term",            required_argument, NULL, 2},
        {"debug",           required_argument, NULL, 3},
        {0, 0, 0, 0}
    };

    zt.fontsize = 1;
    zt.term = "xterm-256color";
    while ((i = getopt_long_only(argc, argv, "", opts, NULL)) != -1) {
        switch(i) {
        case 1:
            if (stod(&zt.fontsize, optarg))
                zt.fontsize = 1;
            break;
        case 2:
            zt.term = optarg;
            break;
        case 3:
            if (stoi(&zt.debug, optarg))
                zt.debug = 0;
            break;
        }
    }

    tinit();
    xfd = xinit();
    tty_init();
    tty_resize();

    tv = to_timespec(500 * MILLISECOND);
    running = 1;
    while (running) {
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        FD_SET(tty, &fds);
        ASSERT((ret = pselect(MAX(xfd, tty)+1, &fds, NULL, NULL,
            &tv, NULL)) >= 0);
        if (!ret) continue;

        if (FD_ISSET(tty, &fds)) {
            tty_read();
            xdraw();
            ldirty_reset();
        }

        if (FD_ISSET(xfd, &fds) && xevent())
            break;
    }

    xfree();
    tfree();
    close(tty);
    return 0;
}
