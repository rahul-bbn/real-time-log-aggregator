#ifndef LOG_READER_H
#define LOG_READER_H

void init_log_reader(const char **files, int count);
void read_new_logs(const char *path);
void push_log_line(const char *line);
int should_read_new(const char *path);

#endif