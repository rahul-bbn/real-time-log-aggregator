#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include "file-watcher.h"
#include "log-reader.h"
#include "merge-engine.h"
#include "iouring-helper.h"

void on_file_change(const char *path)
{
    printf("[WATCHER] File changed: %s\n", path);
    read_new_logs(path);
}

int main()
{
    const char *files[] = {
        "logs/app1.log",
        "logs/app2.log"};

    init_log_reader(files, 2);
    start_merge_engine();
    start_file_watcher(files, 2, on_file_change);

    printf("Merge engine running...\n");
    printf("Write lines like: 2025-12-03 16:00:01 message to logs/app1.log\n");

    while (1)
    {
        sleep(1);
    }

    io_uring_shutdown_global();
    return 0;
}
