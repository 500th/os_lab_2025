#include <stdbool.h>
// Общие функции вынесены в отдельную библиотеку.
#include "common.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  // Результат вычисления отдельного потока.
  uint64_t result;
};

// Вычисление произведения чисел заданного диапазона по модулю.
uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1 % args->mod;
  if (args->begin > args->end)
    return ans;

  for (uint64_t i = args->begin; ; i++) {
    ans = MultModulo(ans, i, args->mod);
    if (i == args->end)
      break;
  }
  return ans;
}

// Каждый поток сохраняет результат в собственную структуру аргументов.
void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  fargs->result = Factorial(fargs);
  return NULL;
}

// Проверяем числовые параметры командной строки.
int ParseInt(const char *text, int min, int max) {
  char *end;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno || *text == '\0' || *end != '\0' || value < min || value > max)
    return -1;
  return (int)value;
}

// Делим полученный диапазон между потоками и объединяем их ответы.
int Calculate(uint64_t begin, uint64_t end, uint64_t mod,
              int tnum, uint64_t *answer) {
  pthread_t *threads = calloc((size_t)tnum, sizeof(*threads));
  struct FactorialArgs *args = calloc((size_t)tnum, sizeof(*args));
  if (!threads || !args) {
    free(threads);
    free(args);
    return -1;
  }

  uint64_t count = end >= begin ? end - begin + 1 : 0;
  uint64_t chunk = count / (uint64_t)tnum;
  uint64_t remainder = count % (uint64_t)tnum;
  uint64_t current = begin;
  int created = 0;

  for (int i = 0; i < tnum; i++) {
    uint64_t size = chunk + ((uint64_t)i < remainder ? 1 : 0);
    args[i].begin = current;
    args[i].end = size ? current + size - 1 : current - 1;
    args[i].mod = mod;
    if (size)
      current += size;

    if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      goto error;
    }
    created++;
  }

  *answer = 1 % mod;
  for (int i = 0; i < created; i++) {
    pthread_join(threads[i], NULL);
    *answer = MultModulo(*answer, args[i].result, mod);
  }
  free(threads);
  free(args);
  return 0;

error:
  for (int i = 0; i < created; i++)
    pthread_join(threads[i], NULL);
  free(threads);
  free(args);
  return -1;
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  // Читаем параметры --port и --tnum, проверяя допустимые значения.
  static struct option options[] = {{"port", required_argument, 0, 0},
                                    {"tnum", required_argument, 0, 0},
                                    {0, 0, 0, 0}};
  int option_index = 0;
  int c;
  while ((c = getopt_long(argc, argv, "", options, &option_index)) != -1) {
    if (c != 0) {
      fprintf(stderr, "Unknown argument\n");
      return 1;
    }
    if (option_index == 0)
      port = ParseInt(optarg, 1, 65535);
    else if (option_index == 1)
      tnum = ParseInt(optarg, 1, 256);
  }
  if (port < 1 || tnum < 1 || optind != argc) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  // Не завершаем сервер по SIGPIPE, если клиент закрыл соединение.
  signal(SIGPIPE, SIG_IGN);
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in server = {0};
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind");
    close(server_fd);
    return 1;
  }
  if (listen(server_fd, 128) < 0) {
    perror("listen");
    close(server_fd);
    return 1;
  }

  printf("Server listening at %d\n", port);
  fflush(stdout);
  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    // Получаем от клиента три uint64_t: begin, end и mod.
    uint64_t task[3];
    if (RecvAll(client_fd, task, sizeof(task)) == 0 &&
        task[0] >= 1 && task[2] > 0 &&
        (task[0] <= task[1] || task[0] == task[1] + 1)) {
      printf("Receive: %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
             task[0], task[1], task[2]);

      uint64_t total;
      if (Calculate(task[0], task[1], task[2], tnum, &total) == 0) {
        printf("Total: %" PRIu64 "\n", total);
        if (SendAll(client_fd, &total, sizeof(total)) < 0)
          perror("send");
      }
    } else {
      fprintf(stderr, "Invalid request or connection closed\n");
    }
    close(client_fd);
  }
  close(server_fd);
  return 0;
}
