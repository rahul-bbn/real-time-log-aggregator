#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

typedef void (*file_change_callback)(const char *path);

void start_file_watcher(const char **files, int file_count, file_change_callback cb);

#endif