
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    // Создание дочернего процесса
    pid_t pid = fork();

    if (pid < 0) {
        printf("Fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // Запуск последовательной программы в дочернем процессе
        execl("./sequential_min_max", "sequential_min_max", "1", "100", (char *)NULL);

        perror("Exec failed");
        return 1;
    } else {
        // Ожидание завершения дочернего процесса
        wait(NULL);
        printf("Child process finished\n");
    }

    return 0;
}
