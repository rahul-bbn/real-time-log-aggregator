#define _GNU_SOURCE
#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <libgen.h>
#include "log-reader.h"
#include "file-watcher.h"

static const char **watch_files;
static int g_file_count;
static file_change_callback g_callback;

typedef struct
{
    char *dir;
    char *filename;
} WatchEntry;

static WatchEntry *entries = NULL;

static void *watch_thread(void *arg)
{
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0)
    {
        perror("inotify_init");
        return NULL;
    }

    entries = calloc(g_file_count, sizeof(WatchEntry));

    for (int i = 0; i < g_file_count; i++)
    {
        char *path_copy = strdup(watch_files[i]);
        char *dir = dirname(path_copy);

        entries[i].dir = strdup(dir);

        char *path_copy2 = strdup(watch_files[i]);
        entries[i].filename = strdup(basename(path_copy2));

        int wd = inotify_add_watch(fd, entries[i].dir,
                                   IN_MODIFY | IN_ATTRIB | IN_MOVED_TO | IN_CLOSE_WRITE | IN_CREATE);

        if (wd < 0)
        {
            perror("inotify_add_watch");
        }

        free(path_copy);
        free(path_copy2);
    }

    char buffer[4096];

    while (1)
    {
        int len = read(fd, buffer, sizeof(buffer));
        if (len <= 0)
        {
            usleep(50 * 1000);
            continue;
        }

        int i = 0;
        while (i < len)
        {
            struct inotify_event *ev = (struct inotify_event *)&buffer[i];

            if (ev->len > 0 && (ev->mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE)))
            {
                for (int j = 0; j < g_file_count; j++)
                {
                    if (strcmp(ev->name, entries[j].filename) == 0)
                    {
                        char fullpath[512];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s",
                                 entries[j].dir, entries[j].filename);

                        if (should_read_new(fullpath))
                        {
                            g_callback(fullpath);
                        }
                        else
                        {
                            // optional debug
                            // printf("[WATCHER] event ignored: %s\n", fullpath);
                        }
                    }
                }
            }

            i += sizeof(*ev) + ev->len;
        }
    }

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
