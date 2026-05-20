#ifndef IPC_H
#define IPC_H

#include <stddef.h>
#include <sys/types.h>

ssize_t pelny_zapis(int fd, const void *buf, size_t n);
ssize_t pelny_odczyt(int fd, void *buf, size_t n);

#endif
