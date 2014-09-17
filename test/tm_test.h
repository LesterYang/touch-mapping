#ifndef _TM_TEST_H
#define _TM_TEST_H

#include <pthread.h>

typedef struct _thread_data_t {
    int ap;
    pthread_t id;
    char* fb;
    char* event;
    int fb_fd;
    int event_fd;
    int con_fd;
} thread_data_t;


int ts_test(thread_data_t* data);
int open_framebuffer(thread_data_t* data);
void close_framebuffer(thread_data_t* data);
void setcolor(unsigned colidx, unsigned value, thread_data_t* data);

#endif
