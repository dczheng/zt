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

struct zt_t zt = {0};
int tty = 0;
pid_t pid;

void ldirty_reset(void);
void cfree(void);
void cinit();
int ctrl(uint8_t*, int);
int xinit(void);
void xfree(void);
int xevent(void);
void xdraw(void);

void
clean(void) {
    xfree();
    cfree();
    close(tty);
    if (zt.log >= 0)
        close(zt.log);
    _exit(0);
}

void
tread(void) {
    static uint8_t buf[BUFSIZ];
    static int n = 0;
    int ret, m;

    ASSERT(n >= 0 && n < (int)sizeof(buf), "");
    if ((ret = read(tty, buf+n, sizeof(buf)-1)) < 0) {
        LOGERR("failed to read tty: %s", strerror(errno));
        return;
    }

    n += ret;
    m = ctrl(buf, n);
    n -= m;
    if (n > 0)
        memmove(buf, buf+m, n);
}

void
twrite(char *s, int n) {
    int ret;
    fd_set fds;
    struct timespec tv;

    tv = to_timespec(SECOND);
    for (;;) {
        if (n <= 0) break;

        FD_ZERO(&fds);
        FD_SET(tty, &fds);
        ASSERT(pselect(tty+1, NULL, &fds, NULL, &tv, NULL) > 0,
            "select failed: %s", strerror(errno));

        if ((ret = write(tty, s, n)) < 0)  {
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
    int ret, i, xfd;
    long now, last, latency, timeout;
    struct timespec tv, *ptv;
    fd_set fds;
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
    xfd = xinit();
    tinit();
    tresize();

    last = get_time();
    timeout = -1;
    latency = LATENCY * MICROSECOND;
    for (;;){
        ptv = NULL;
        if (timeout > 0) {
            tv = to_timespec(timeout);
            ptv = &tv;
        }

        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        FD_SET(tty, &fds);
        ASSERT((ret = pselect(MAX(xfd, tty)+1, &fds, NULL, NULL,
            ptv, NULL)) >= 0, "select failed: %s", strerror(errno));
        if (!ret) continue;

        if (FD_ISSET(xfd, &fds) && xevent())
            break;

        if (FD_ISSET(tty, &fds))
            tread();

        now = get_time();
        latency -= now-last;
        last = now;
        //printf("%ld\n", latency);

        if (latency >= 0) {
            timeout = latency;
            continue;
        }

        latency = LATENCY * MICROSECOND;
        timeout = -1;
        xdraw();
        ldirty_reset();

    }

    clean();
    return 0;
}
