#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include "sum.h"
#include "utils.h"

// Выполнение подсчёта суммы в отдельном потоке
void *ThreadSum(void *arg) {
    struct SumArgs *sum_args = (struct SumArgs *)arg;
    sum_args->result = Sum(sum_args);
    return NULL;
}

int main(int argc, char **argv) {
    int threads_num = 0;
    int array_size = 0;
    int seed = 0;

    // Чтение аргументов командной строки
    static struct option options[] = {
        {"threads_num", required_argument, 0, 't'},
        {"seed", required_argument, 0, 's'},
        {"array_size", required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    int option;
    while ((option = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (option) {
            case 't': threads_num = atoi(optarg); break;
            case 's': seed = atoi(optarg); break;
            case 'a': array_size = atoi(optarg); break;
            default: return 1;
        }
    }

    if (threads_num <= 0 || seed <= 0 || array_size <= 0 || optind != argc) {
        printf("Usage: ./psum --threads_num N --seed N --array_size N\n");
        return 1;
    }

    if (threads_num > array_size) {
        printf("Threads num must not exceed array size\n");
        return 1;
    }

    int *array = malloc(sizeof(int) * (size_t)array_size);
    pthread_t *threads = malloc(sizeof(pthread_t) * (size_t)threads_num);
    struct SumArgs *args = malloc(sizeof(struct SumArgs) * (size_t)threads_num);
    if (array == NULL || threads == NULL || args == NULL) {
        printf("Memory allocation failed\n");
        free(array);
        free(threads);
        free(args);
        return 1;
    }

    // Генерация массива выполняется до начала измерения времени
    GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Распределение участков массива между потоками
    int created = 0;
    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = (int)((long long)i * array_size / threads_num);
        args[i].end = (int)((long long)(i + 1) * array_size / threads_num);
        args[i].result = 0;

        if (pthread_create(&threads[i], NULL, ThreadSum, &args[i]) != 0) {
            printf("Error: pthread_create failed!\n");
            break;
        }
        created++;
    }

    long long total_sum = 0;
    for (int i = 0; i < created; i++) {
        pthread_join(threads[i], NULL);
        total_sum += args[i].result;
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed_ms = (finish.tv_sec - start.tv_sec) * 1000.0 +
                        (finish.tv_nsec - start.tv_nsec) / 1000000.0;

    if (created == threads_num) {
        printf("Total: %lld\n", total_sum);
        printf("Elapsed time: %.6fms\n", elapsed_ms);
    }

    free(array);
    free(threads);
    free(args);
    return created == threads_num ? 0 : 1;
}
