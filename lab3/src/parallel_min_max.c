#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);

            // Проверка начального числа генератора
            if (seed <= 0) {
              printf("Seed must be positive\n");
              return 1;
            }

            break;
          case 1:
            array_size = atoi(optarg);

            // Проверка размера массива
            if (array_size <= 0) {
              printf("Array size must be positive\n");
              return 1;
            }

            break;
          case 2:
            pnum = atoi(optarg);

            // Проверка количества процессов
            if (pnum <= 0) {
              printf("Pnum must be positive\n");
              return 1;
            }

            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  // Проверка распределения массива между процессами
  if (pnum > array_size) {
    printf("Too many processes\n");
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);

  // Проверка выделения памяти
  if (array == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Создание каналов для передачи результатов
  int pipes[pnum][2];

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe failed!\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process

        // Распределение массива и поиск минимума и максимума
        int begin = i * array_size / pnum;
        int end = (i + 1) * array_size / pnum;
        struct MinMax result = GetMinMax(array, begin, end);

        if (with_files) {
          // Запись результата в файл
          char filename[100];
          sprintf(filename, "result_%d.txt", i);

          FILE *file = fopen(filename, "w");
          if (file == NULL) {
            printf("File open failed\n");
            return 1;
          }

          fprintf(file, "%d %d\n", result.min, result.max);
          fclose(file);

        } else {
          // Передача результата через pipe
          for (int j = 0; j < pnum; j++) {
            close(pipes[j][0]);
            if (j != i) {
              close(pipes[j][1]);
            }
          }

          if (write(pipes[i][1], &result, sizeof(result))
              != sizeof(result)) {
            printf("Pipe write failed\n");
            return 1;
          }

          close(pipes[i][1]);
        }
        return 0;
      }

      // Закрытие записи в pipe родительским процессом
      if (!with_files) {
        close(pipes[i][1]);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  while (active_child_processes > 0) {
    // Ожидание завершения дочерних процессов
    wait(NULL);

    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Чтение результата из файла
      char filename[100];
      sprintf(filename, "result_%d.txt", i);

      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        printf("File read failed\n");
        return 1;
      }

      if (fscanf(file, "%d %d", &min, &max) != 2) {
        printf("Invalid file data\n");
        fclose(file);
        return 1;
      }

      fclose(file);
      remove(filename);

    } else {
      // Чтение результата из pipe
      struct MinMax result;

      if (read(pipes[i][0], &result, sizeof(result))
          != sizeof(result)) {
        printf("Pipe read failed\n");
        return 1;
      }

      close(pipes[i][0]);

      min = result.min;
      max = result.max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
