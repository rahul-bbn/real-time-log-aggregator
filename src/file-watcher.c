#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "file-watcher.h"

static const char **watch_files;
static int file_count;
static file_change_callback callback;

static void *watch_thread(void *arg)
{
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        return NULL;
    }

    int wd[file_count];
    for (int i = 0; i < file_count; i++)
    {
        wd[i] = inotify_add_watch(fd, watch_files[i], IN_MODIFY);
    }

    char buffer[1024];
    while (1)
    {
        int len = read(fd, buffer, sizeof(buffer));
        int i = 0;

        while (i < len)
        {
            struct inotify_event *ev = (struct inotify_event *)&buffer[i];
            if (ev->mask & IN_MODIFY)
            {
                callback(watch_files[ev->wd - 1]);
            }
            i += sizeof(*ev) + ev->len;
        }
    }
    return NULL;
}

void start_file_watcher(const char **files, int count, file_change_callback cb)
{
    watch_files = files;
    file_count = count;
    callback = cb;

    pthread_t t;
    pthread_create(&t, NULL, watch_thread, NULL);
    pthread_detach(t);
}
