#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>

int main(void) {
    printf("booting...\n");
    mount("proc", "/proc", "proc", 0, "");
    mount("sysfs", "/sys", "sysfs", 0, "");
    mount("devtmpfs", "/dev", "devtmpfs", 0, "");
    system("echo /sbin/mdev > /proc/sys/kernel/hotplug");
    system("/sbin/mdev -s");
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/xsh", "/bin/xsh", NULL);
        perror("execl failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("shell exited with code %d\n", WEXITSTATUS(status));
    }
    while (1) pause();
}