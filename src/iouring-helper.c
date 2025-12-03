#include <stdio.h>
#include <unistd.h>
#include "iouring-helper.h"

static struct io_uring ring;

int io_uring_init_global()
{
    return io_uring_queue_init(256, &ring, 0);
}

int io_uring_submit_read(int fd, char *buf, size_t size, off_t offset)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
        return -1;

    io_uring_prep_read(sqe, fd, buf, size, offset);

    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring, &cqe);

    int ret = cqe->res;
    io_uring_cqe_seen(&ring, cqe);

    return ret;
}
