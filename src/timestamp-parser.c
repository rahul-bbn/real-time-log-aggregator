#include "timestamp-parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

time_t parse_timestamp(const char *s)
{
    int y, M, d, h, m, sec;
    sscanf(s, "%d-%d-%d %d:%d:%d", &y, &M, &d, &h, &m, &sec);

    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = M - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    t.tm_min = m;
    t.tm_sec = sec;

    return mktime(&t);
}