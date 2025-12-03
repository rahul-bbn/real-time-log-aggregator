#ifndef LOG_READER_H
#define LOG_READER_H

void init_log_reader(const char **files, int count);
void read_new_logs(const char *path);

#endif