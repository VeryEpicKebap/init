#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

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

void spawn_sh(const char *path) {
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
    printf("signal %d, poweroff...\n", sig);
    sync();
    reboot(RB_POWER_OFF);
}

int main(void) {
    printf("xylen-init (0.2b)\n - Welcome! -");
    mount_fs("proc", "/proc", "proc");
    mount_fs("sysfs", "/sys", "sysfs");
    mount_fs("devtmpfs", "/dev", "devtmpfs");
    setup_mdev();
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    spawn_sh("/bin/xsh");
    spawn_sh("/bin/sh");
    while (1) pause();
}