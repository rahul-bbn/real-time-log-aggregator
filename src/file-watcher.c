#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "file-watcher.h"

static const char **watch_files = NULL;
static int file_count = 0;
static file_change_callback callback = NULL;

static void *watch_thread(void *arg)
{
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        return NULL;
    }

    int *wd = calloc(file_count, sizeof(int));
    if (!wd)
    {
        perror("calloc");
        close(fd);
        return NULL;
    }

    for (int i = 0; i < file_count; i++)
    {
        wd[i] = inotify_add_watch(fd, watch_files[i], IN_MODIFY);
        if (wd[i] < 0)
        {
            fprintf(stderr, "inotify_add_watch failed for %s: %s\n", watch_files[i], strerror(errno));
            wd[i] = -1;
        }
    }

    const size_t buf_len = (sizeof(struct inotify_event) + 256) * 16;
    char *buffer = malloc(buf_len);
    if (!buffer)
    {
        perror("malloc");
        free(wd);
        close(fd);
        return NULL;
    }

    while (1)
    {
        ssize_t length = read(fd, buffer, buf_len);
        if (length < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(100 * 1000);
                continue;
            }
            perror("read(inotify)");
            break;
        }
        if (length == 0)
            continue;

        ssize_t i = 0;
        while (i < length)
        {
            struct inotify_event *ev = (struct inotify_event *)&buffer[i];
            if (ev->mask & IN_MODIFY)
            {
                int idx = -1;
                for (int j = 0; j < file_count; ++j)
                {
                    if (wd[j] == ev->wd)
                    {
                        idx = j;
                        break;
                    }
                }
                if (idx != -1)
                {
                    if (callback)
                        callback(watch_files[idx]);
                }
            }
            i += sizeof(struct inotify_event) + ev->len;
        }
    }

    free(buffer);
    free(wd);
    close(fd);
    return NULL;
}

void start_file_watcher(const char **files, int count, file_change_callback cb)
{
    if (!files || count <= 0 || !cb)
        return;

    watch_files = files;
    file_count = count;
    callback = cb;

    pthread_t t;
    if (pthread_create(&t, NULL, watch_thread, NULL) != 0)
    {
        perror("pthread_create");
        return;
    }
    pthread_detach(t);
}