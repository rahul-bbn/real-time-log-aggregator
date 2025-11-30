#include <stdio.h>
#include <unistd.h>
#include "file-watcher.h"
#include "log-reader.h"

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
    start_file_watcher(files, 2, on_file_change);

    printf("Day 2 watcher + log reader running...\n");

    while (1)
    {
        sleep(1);
    }

    return 0;
}