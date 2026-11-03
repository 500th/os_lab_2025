#include "common.h"
#include <errno.h>
#include <sys/socket.h>

// Безопасно умножаем два числа по модулю без переполнения uint64_t.
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  return (uint64_t)(((__uint128_t)a * b) % mod);
}

// Получаем ровно size байт: TCP может доставлять сообщение частями.
int RecvAll(int fd, void *buffer, size_t size) {
  size_t received = 0;
  while (received < size) {
    ssize_t n = recv(fd, (char *)buffer + received, size - received, 0);
    if (n == 0)
      return -1;
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    received += (size_t)n;
  }
  return 0;
}

// Отправляем все байты: один send не гарантирует передачу всего сообщения.
int SendAll(int fd, const void *buffer, size_t size) {
  size_t sent = 0;
  while (sent < size) {
    ssize_t n = send(fd, (const char *)buffer + sent, size - sent, 0);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      return -1;
    sent += (size_t)n;
  }
  return 0;
}
