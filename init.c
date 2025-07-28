
// xylen-init (0.3c)
// written by VeryEpicKebap

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <sys/ioctl.h>
#include <termios.h>

void mount_fs(const char *src, const char *target, const char *fstype) {
    if (mount(src, target, fstype, 0, "") < 0) {
        if (errno != EBUSY) {
            fprintf(stderr, "mount %s on %s: %s\n", fstype, target, strerror(errno));
        }
    }
}

void setup_mdev() {
    FILE *fp = fopen("/proc/sys/kernel/hotplug", "w");
    if (fp) {
        fprintf(fp, "/sbin/mdev\n");
        fclose(fp);
        system("/sbin/mdev -s");
    }
}

void launch_tty(const char *tty, const char *shell) {
    pid_t pid = fork();
    if (pid == 0) {
        int fd = open(tty, O_RDWR);
        if (fd < 0) {
            fprintf(stderr, "open %s: %s\n", tty, strerror(errno));
            _exit(1);
        }

        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        setsid();
        ioctl(STDIN_FILENO, TIOCSCTTY, 0);

        execl(shell, shell, NULL);
        fprintf(stderr, "exec %s: %s\n", shell, strerror(errno));
        _exit(1);
    }
}

void sig_handler(int sig) {
    sync();
    reboot(RB_POWER_OFF);
}

int main(void) {
    printf("\x1b[36m [ xylen-init (v0.3c) ] \x1b[0m \n\n\x1b[32mWelcome!\x1b[0m\n");

    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) < 0) {
        fprintf(stderr, "remount / rw: %s\n", strerror(errno));
    }

    mount_fs("proc", "/proc", "proc");
    mount_fs("sysfs", "/sys", "sysfs");
    mount_fs("devtmpfs", "/dev", "devtmpfs");
    setup_mdev();

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    const char *shell = "/bin/xsh";
    struct stat st;
    if (stat(shell, &st) != 0)
        shell = "/bin/sh";

    launch_tty("/dev/tty1", shell);
    launch_tty("/dev/tty2", shell);

    while (1) {
        int status;
        wait(&status);
    }

    return 0;
}