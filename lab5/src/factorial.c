
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

typedef struct {
    unsigned long long start;
    unsigned long long end;
    unsigned long long mod;
} ThreadArgs;

unsigned long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Вычисление части факториала в отдельном потоке
void *calculate_factorial(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    unsigned long long partial = 1;

    for (unsigned long long i = args->start; i <= args->end; i++) {
        partial = (partial * i) % args->mod;
    }

    pthread_mutex_lock(&mutex);
    result = (result * partial) % args->mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

// Получение параметров и запуск потоков
int main(int argc, char *argv[]) {
    unsigned long long k = 0;
    unsigned long long mod = 0;
    int pnum = 0;
    int option;

    struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    while ((option = getopt_long(argc, argv, "k:", long_options, NULL)) != -1) {
        switch (option) {
            case 'k':
                k = strtoull(optarg, NULL, 10);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = strtoull(optarg, NULL, 10);
                break;
            default:
                return 1;
        }
    }

    if (pnum <= 0 || mod == 0) {
        printf("Invalid arguments\n");
        return 1;
    }

    if (k > 0 && (unsigned long long)pnum > k) {
        pnum = (int)k;
    }

    pthread_t threads[pnum];
    ThreadArgs args[pnum];

    unsigned long long chunk = k / pnum;
    unsigned long long remainder = k % pnum;
    unsigned long long start = 1;

    result = 1 % mod;

    for (int i = 0; i < pnum; i++) {
        unsigned long long count = chunk + (i < remainder ? 1 : 0);

        args[i].start = start;
        args[i].end = start + count - 1;
        args[i].mod = mod;

        pthread_create(&threads[i], NULL, calculate_factorial, &args[i]);

        start += count;
    }

    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Result: %llu\n", result);

    pthread_mutex_destroy(&mutex);

    return 0;
}
