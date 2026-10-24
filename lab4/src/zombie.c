#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("Child PID: %ld. Child exits.\n", (long)getpid());
        fflush(stdout);
        exit(0);
    }

    // Родитель не вызывает wait(), поэтому дочерний процесс временно станет зомби
    printf("Parent PID: %ld, Child PID: %ld\n", (long)getpid(), (long)pid);
    printf("Check zombie: ps -o pid,ppid,stat,cmd -p %ld\n", (long)pid);
    fflush(stdout);
    sleep(20);

    // Забираем статус завершения дочернего процесса, удаляя запись о зомби
    waitpid(pid, NULL, 0);
    printf("Child reaped. Zombie removed.\n");
    return 0;
}
