// xylen-init (0.3a)
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

void spawn_shell(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(stderr, "no %s (%s)\n", path, strerror(errno));
        return;
    }
    if (!(st.st_mode & S_IXUSR)) {
        fprintf(stderr, "not executable: %s\n", path);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        int fd = -1;
        const char *tty_devices[] = {"/dev/tty1", "/dev/ttyS0", "/dev/console", NULL};
        
        for (int i = 0; tty_devices[i]; i++) {
            fd = open(tty_devices[i], O_RDWR);
            if (fd >= 0) break;
        }

        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            setsid();
            ioctl(fd, TIOCSCTTY, 0);
            close(fd);
        }

        execl(path, path, NULL);
        fprintf(stderr, "exec %s: %s\n", path, strerror(errno));
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
}

void sig_handler(int sig) {
    sync();
    reboot(RB_POWER_OFF);
}

int main(void) {
    printf("\x1b[33m xylen-init (0.2c) \x1b[0m \n\n  \x1b[32m Welcome!\x1b[0m\n");
    mount_fs("proc", "/proc", "proc");
    mount_fs("sysfs", "/sys", "sysfs");
    mount_fs("devtmpfs", "/dev", "devtmpfs");
    setup_mdev();

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    while (1) {
        spawn_shell("/bin/xsh");
        spawn_shell("/bin/sh");
        sleep(1);
    }

    return 0;
}