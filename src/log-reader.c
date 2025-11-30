#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "log-reader.h"

typedef struct
{
    const char *path;
    size_t prev_size;
} LogFile;

static LogFile *log_files;
static int log_count = 0;

void init_log_reader(const char **files, int count)
{
    log_files = malloc(sizeof(LogFile) * count);
    log_count = count;
    for (int i = 0; i < count; i++)
    {
        log_files[i].path = files[i];
        log_files[i].prev_size = 0;
    }
}

void read_new_logs(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return;

    struct stat st;
    fstat(fd, &st);

    size_t prev = 0;
    for (int i = 0; i < log_count; i++)
    {
        if (strcmp(log_files[i].path, path) == 0)
        {
            prev = log_files[i].prev_size;
            break;
        }
    }

    if (st.st_size <= prev)
    {
        close(fd);
        return;
    }

    size_t new_size = st.st_size - prev;

    char *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        close(fd);
        return;
    }

    // Print new lines
    ssize_t bytes_written = write(1, data + prev, new_size);
    if (bytes_written != new_size)
    {
        perror("write failed");
    }

    // Update prev_size
    for (int i = 0; i < log_count; i++)
    {
        if (strcmp(log_files[i].path, path) == 0)
        {
            log_files[i].prev_size = st.st_size;
            break;
        }
    }

    munmap(data, st.st_size);
    close(fd);
}
