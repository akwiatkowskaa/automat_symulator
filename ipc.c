#include "ipc.h"

#include <unistd.h>

ssize_t pelny_zapis(int fd, const void *buf, size_t n)
{
	const char *p = (const char *)buf;
	size_t zostalo = n;
	while (zostalo > 0) {
		ssize_t w = write(fd, p, zostalo);
		if (w <= 0)
			return w;
		p += (size_t)w;
		zostalo -= (size_t)w;
	}
	return (ssize_t)n;
}

ssize_t pelny_odczyt(int fd, void *buf, size_t n)
{
	char *p = (char *)buf;
	size_t zostalo = n;
	while (zostalo > 0) {
		ssize_t r = read(fd, p, zostalo);
		if (r <= 0)
			return r;
		p += (size_t)r;
		zostalo -= (size_t)r;
	}
	return (ssize_t)n;
}
