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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

struct Server {
  char ip[255];
  int port;
};

// Данные одного серверного соединения, выполняемого в отдельном потоке.
struct ServerTask {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int success;
};

// Строго проверяем значение беззнакового 64-битного числа.
bool ConvertStringToUI64(const char *str, uint64_t *val) {
  if (*str == '\0' || *str == '-')
    return false;
  char *end = NULL;
  errno = 0;
  unsigned long long value = strtoull(str, &end, 10);
  if (errno || *end != '\0')
    return false;
  *val = (uint64_t)value;
  return true;
}

// Загружаем адреса серверов из файла: по одному ip:port в строке.
int ReadServers(const char *path, struct Server **servers, size_t *count) {
  FILE *file = fopen(path, "r");
  if (!file) {
    perror("fopen");
    return -1;
  }
  char line[512];
  *servers = NULL;
  *count = 0;
  while (fgets(line, sizeof(line), file)) {
    if (!strchr(line, '\n') && !feof(file)) {
      fprintf(stderr, "Server address line too long\n");
      goto error;
    }
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\n' || *p == '\0' || *p == '#')
      continue;

    struct Server item;
    char extra;
    if (sscanf(p, " %254[^:]:%d %c", item.ip, &item.port, &extra) != 2 ||
        item.port < 1 || item.port > 65535) {
      fprintf(stderr, "Invalid server address: %s", line);
      goto error;
    }
    struct Server *next = realloc(*servers, (*count + 1) * sizeof(**servers));
    if (!next) {
      perror("realloc");
      goto error;
    }
    *servers = next;
    (*servers)[*count] = item;
    (*count)++;
  }
  if (ferror(file) || *count == 0) {
    fprintf(stderr, "Server list is empty or unreadable\n");
    goto error;
  }
  fclose(file);
  return 0;

error:
  free(*servers);
  *servers = NULL;
  *count = 0;
  fclose(file);
  return -1;
}

// Отдельный поток подключается к своему серверу и получает частичный результат.
void *ContactServer(void *arg) {
  struct ServerTask *task = (struct ServerTask *)arg;
  // Преобразуем IPv4-адрес без gethostbyname, небезопасной для потоков.
  struct sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_port = htons((uint16_t)task->server.port);
  if (inet_pton(AF_INET, task->server.ip, &address.sin_addr) != 1) {
    fprintf(stderr, "Invalid IPv4 address: %s\n", task->server.ip);
    return NULL;
  }

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    perror("socket");
    return NULL;
  }
  if (connect(sck, (struct sockaddr *)&address, sizeof(address)) < 0) {
    fprintf(stderr, "Connection failed: %s:%d\n", task->server.ip, task->server.port);
    close(sck);
    return NULL;
  }

  uint64_t request[3] = {task->begin, task->end, task->mod};
  if (SendAll(sck, request, sizeof(request)) < 0 ||
      RecvAll(sck, &task->result, sizeof(task->result)) < 0) {
    fprintf(stderr, "Communication failed: %s:%d\n", task->server.ip, task->server.port);
    close(sck);
    return NULL;
  }
  task->success = 1;
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0, mod = 0;
  char servers_path[255] = {0};
  bool has_k = false, has_mod = false;

  // Получаем --k, --mod и путь к файлу --servers.
  static struct option options[] = {{"k", required_argument, 0, 0},
                                    {"mod", required_argument, 0, 0},
                                    {"servers", required_argument, 0, 0},
                                    {0, 0, 0, 0}};
  int option_index = 0;
  int c;
  while ((c = getopt_long(argc, argv, "", options, &option_index)) != -1) {
    if (c != 0) {
      fprintf(stderr, "Arguments error\n");
      return 1;
    }
    switch (option_index) {
      case 0: has_k = ConvertStringToUI64(optarg, &k); break;
      case 1: has_mod = ConvertStringToUI64(optarg, &mod); break;
      case 2:
        if (strlen(optarg) >= sizeof(servers_path)) {
          fprintf(stderr, "Server file path is too long\n");
          return 1;
        }
        strcpy(servers_path, optarg);
        break;
    }
  }
  if (!has_k || !has_mod || mod == 0 || !servers_path[0] || optind != argc) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
    return 1;
  }

  struct Server *servers = NULL;
  size_t servers_num = 0;
  if (ReadServers(servers_path, &servers, &servers_num) < 0)
    return 1;

  struct ServerTask *tasks = calloc(servers_num, sizeof(*tasks));
  pthread_t *threads = calloc(servers_num, sizeof(*threads));
  if (!tasks || !threads) {
    fprintf(stderr, "Not enough memory\n");
    free(tasks); free(threads); free(servers);
    return 1;
  }

  // Делим числа от 1 до k между серверами максимально равномерно.
  uint64_t chunk = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current = 1;
  size_t created = 0;
  signal(SIGPIPE, SIG_IGN);
  for (size_t i = 0; i < servers_num; i++) {
    uint64_t size = chunk + (i < remainder ? 1 : 0);
    tasks[i].server = servers[i];
    tasks[i].begin = current;
    tasks[i].end = size ? current + size - 1 : current - 1;
    tasks[i].mod = mod;
    if (size)
      current += size;

    if (pthread_create(&threads[i], NULL, ContactServer, &tasks[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      break;
    }
    created++;
  }

  // Ожидаем все потоки и объединяем ответы серверов по модулю.
  uint64_t answer = 1 % mod;
  int ok = created == servers_num;
  for (size_t i = 0; i < created; i++) {
    pthread_join(threads[i], NULL);
    if (!tasks[i].success) {
      ok = 0;
      continue;
    }
    printf("Server %s:%d (%" PRIu64 "..%" PRIu64 "): %" PRIu64 "\n",
           tasks[i].server.ip, tasks[i].server.port,
           tasks[i].begin, tasks[i].end, tasks[i].result);
    answer = MultModulo(answer, tasks[i].result, mod);
  }
  if (ok)
    printf("answer: %" PRIu64 "\n", answer);
  else
    fprintf(stderr, "Some servers failed; no final answer\n");

  free(threads);
  free(tasks);
  free(servers);
  return ok ? 0 : 1;
}
