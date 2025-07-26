// xylen-init (0.2c)
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
    if (mount(src, target, fstype, 0, "") < 0)
        fprintf(stderr, "mount %s on %s: %s\n", fstype, target, strerror(errno));
    else
        printf("mounted %s on %s\n", fstype, target);
}

void setup_mdev() {
    FILE *fp = fopen("/proc/sys/kernel/hotplug", "w");
    if (fp) {
        fprintf(fp, "/sbin/mdev\n");
        fclose(fp);
        system("/sbin/mdev -s");
    } else {
        perror("hotplug");
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
        int fd = open("/dev/tty1", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            setsid();
            ioctl(fd, TIOCSCTTY, 0);
        } else {
            perror("open /dev/tty1");
        }
        execl(path, path, NULL);
        fprintf(stderr, "exec %s: %s\n", path, strerror(errno));
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("exit %d\n", WEXITSTATUS(status));
    } else {
        perror("fork");
    }
}

void sig_handler(int sig) {
    printf("signal %d received, powering off...\n", sig);
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