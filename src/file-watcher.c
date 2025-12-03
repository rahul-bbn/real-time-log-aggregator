#define _GNU_SOURCE
#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "file-watcher.h"

static const char **watch_files;
static int g_file_count;
static file_change_callback g_callback;

typedef struct {
    int wd;
    const char *path;
} WatchEntry;

static WatchEntry *watch_entries = NULL;

static void *watch_thread(void *arg)
{
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0)
    {
        perror("inotify_init");
        return NULL;
    }

    watch_entries = calloc(g_file_count, sizeof(WatchEntry));
    if (!watch_entries) {
        perror("calloc");
        close(fd);
        return NULL;
    }

    for (int i = 0; i < g_file_count; i++)
    {
        int wd = inotify_add_watch(fd, watch_files[i], IN_MODIFY | IN_ATTRIB);
        if (wd < 0) {
            perror("inotify_add_watch");
            wd = -1;
        }
        watch_entries[i].wd = wd;
        watch_entries[i].path = watch_files[i];
    }

    char buffer[4096];
    while (1)
    {
        int len = read(fd, buffer, sizeof(buffer));
        if (len <= 0) {
            usleep(100 * 1000);
            continue;
        }

        int i = 0;
        while (i < len)
        {
            struct inotify_event *ev = (struct inotify_event *)&buffer[i];
            if (ev->wd != 0 && (ev->mask & (IN_MODIFY | IN_ATTRIB)))
            {
                for (int j = 0; j < g_file_count; j++) {
                    if (watch_entries[j].wd == ev->wd) {
                        g_callback(watch_entries[j].path);
                        break;
                    }
                }
            }
            i += sizeof(*ev) + ev->len;
        }
    }

    close(fd);
    free(watch_entries);
    return NULL;
}

void start_file_watcher(const char **files, int count, file_change_callback cb)
{
    watch_files = files;
    g_file_count = count;
    g_callback = cb;

    pthread_t t;
    pthread_create(&t, NULL, watch_thread, NULL);
    pthread_detach(t);
}