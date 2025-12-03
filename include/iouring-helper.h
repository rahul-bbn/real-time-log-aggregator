#ifndef IO_URING_HELPER_H
#define IO_URING_HELPER_H

#include <liburing.h>
#include <stddef.h>

int io_uring_init_global();
int io_uring_submit_read(int fd, char *buf, size_t size, off_t offset);

#endif
