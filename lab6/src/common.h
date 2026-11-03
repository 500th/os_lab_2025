#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

// Общие функции клиента и сервера.
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
int RecvAll(int fd, void *buffer, size_t size);
int SendAll(int fd, const void *buffer, size_t size);

#endif
