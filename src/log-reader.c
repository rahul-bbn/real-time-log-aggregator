#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "log-reader.h"
#include "merge-engine.h"
#include "iouring-helper.h"

typedef struct
{
    const char *path;
    size_t prev_size;
} LogFile;

static LogFile *log_files = NULL;
static int log_count = 0;

static pthread_mutex_t prev_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_log_reader(const char **files, int count)
{
    if (log_files) free(log_files);
    log_files = malloc(sizeof(LogFile) * count);
    log_count = count;
    for (int i = 0; i < count; i++)
    {
        log_files[i].path = files[i];
        struct stat st;
        if (stat(files[i], &st) == 0)
            log_files[i].prev_size = st.st_size;
        else
            log_files[i].prev_size = 0;
    }

    io_uring_init_global();
}

static int find_log_index(const char *path)
{
    for (int i = 0; i < log_count; i++)
    {
        if (strcmp(log_files[i].path, path) == 0)
            return i;
    }
    return -1;
}

int should_read_new(const char *path)
{
    int idx = find_log_index(path);
    if (idx < 0) return 1;

    struct stat st;
    if (stat(path, &st) < 0) return 0;

    pthread_mutex_lock(&prev_mutex);
    size_t prev = log_files[idx].prev_size;
    int need = (st.st_size > prev) ? 1 : 0;
    pthread_mutex_unlock(&prev_mutex);
    return need;
}

static void update_prev_size_for_path(const char *path, size_t new_prev)
{
    int idx = find_log_index(path);
    if (idx < 0) return;
    pthread_mutex_lock(&prev_mutex);
    log_files[idx].prev_size = new_prev;
    pthread_mutex_unlock(&prev_mutex);
}

void read_new_logs(const char *path)
{
    printf("[DEBUG] schedule_read_new_logs called for %s\n", path);
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return;
    }

    size_t prev = 0;
    int idx = find_log_index(path);
    if (idx >= 0) {
        pthread_mutex_lock(&prev_mutex);
        prev = log_files[idx].prev_size;
        pthread_mutex_unlock(&prev_mutex);
    }

    if (st.st_size <= prev)
    {
        close(fd);
        return;
    }

    size_t new_size = st.st_size - prev;
    if (new_size == 0) {
        close(fd);
        return;
    }

    char *buf = malloc(new_size + 1);
    if (!buf) {
        close(fd);
        return;
    }

    ssize_t r = io_uring_submit_read(fd, buf, new_size, (off_t)prev);
    if (r < 0) {
        ssize_t got = pread(fd, buf, new_size, (off_t)prev);
        if (got < 0) {
            free(buf);
            close(fd);
            return;
        }
        r = got;
    }

    size_t got = (size_t) r;
    buf[got] = '\0';

    char *start = buf;
    char *end = buf + got;
    while (start < end)
    {
        char *newline = memchr(start, '\n', end - start);
        if (!newline) {
            size_t len = end - start;
            if (len > 0) {
                char *line = malloc(len + 2);
                memcpy(line, start, len);
                line[len] = '\n';
                line[len+1] = '\0';
                push_log_line(line);
                free(line);
            }
            break;
        }
        size_t len = newline - start + 1;
        char *line = malloc(len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        push_log_line(line);
        free(line);
        start = newline + 1;
    }

    if (idx >= 0) update_prev_size_for_path(path, st.st_size);

    free(buf);
    close(fd);
}
