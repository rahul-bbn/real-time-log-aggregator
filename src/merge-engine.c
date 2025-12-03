#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "merge-engine.h"
#include "timestamp-parser.h"

typedef struct
{
    time_t ts;
    char *line;
} HeapNode;

#define MAX_HEAP 8192
static HeapNode heap[MAX_HEAP];
static int heap_size = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void heap_swap(int i, int j)
{
    HeapNode tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
}

static void heap_insert(HeapNode node)
{
    if (heap_size + 1 >= MAX_HEAP) {
        free(node.line);
        return;
    }
    heap[++heap_size] = node;
    int i = heap_size;
    while (i > 1 && heap[i].ts < heap[i / 2].ts)
    {
        heap_swap(i, i / 2);
        i /= 2;
    }
}

static HeapNode heap_pop()
{
    HeapNode top = heap[1];
    heap[1] = heap[heap_size--];
    int i = 1;
    while (1)
    {
        int left = i * 2;
        int right = left + 1;
        int smallest = i;
        if (left <= heap_size && heap[left].ts < heap[smallest].ts) smallest = left;
        if (right <= heap_size && heap[right].ts < heap[smallest].ts) smallest = right;
        if (smallest == i) break;
        heap_swap(i, smallest);
        i = smallest;
    }
    return top;
}

void push_log_line(const char *line)
{
    pthread_mutex_lock(&lock);

    HeapNode node;
    node.ts = parse_timestamp(line);
    node.line = strdup(line);
    if (node.line == NULL) {
        pthread_mutex_unlock(&lock);
        return;
    }

    heap_insert(node);
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void *merge_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        pthread_mutex_lock(&lock);
        while (heap_size == 0)
            pthread_cond_wait(&cond, &lock);

        HeapNode n = heap_pop();
        pthread_mutex_unlock(&lock);
        printf("[MERGED] %s", n.line);
        free(n.line);
    }
    return NULL;
}

void start_merge_engine()
{
    pthread_t tid;
    pthread_create(&tid, NULL, merge_thread, NULL);
    pthread_detach(tid);
}