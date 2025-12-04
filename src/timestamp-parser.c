#define _GNU_SOURCE
#include "timestamp-parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

time_t parse_timestamp(const char *s)
{
    int y=0, M=0, d=0, h=0, m=0, sec=0;
    if (sscanf(s, "%d-%d-%d %d:%d:%d", &y, &M, &d, &h, &m, &sec) != 6) {
        return time(NULL);
    }
    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = M - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    t.tm_min = m;
    t.tm_sec = sec;
    
    time_t tt = mktime(&t);
    if (tt == (time_t)-1) return time(NULL);
    return tt;
}
