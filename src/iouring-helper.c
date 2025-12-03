#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <liburing.h>
#include <unistd.h>
#include "iouring-helper.h"

static struct io_uring ring;
static int ring_inited = 0;

int io_uring_init_global()
{
    if (ring_inited) return 0;
    int ret = io_uring_queue_init(256, &ring, 0);
    if (ret == 0) ring_inited = 1;
    return ret;
}

int io_uring_submit_read(int fd, char *buf, size_t size, off_t offset)
{
    if (!ring_inited) {
        if (io_uring_init_global() != 0) return -1;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
        return -1;

    io_uring_prep_read(sqe, fd, buf, size, offset);
    io_uring_sqe_set_data(sqe, NULL);

    int ret = io_uring_submit(&ring);
    if (ret < 0) return ret;

    struct io_uring_cqe *cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) return ret;

    int res = cqe->res;
    io_uring_cqe_seen(&ring, cqe);
    return res;
}

void io_uring_shutdown_global()
{
    if (ring_inited) {
        io_uring_queue_exit(&ring);
        ring_inited = 0;
    }
}
