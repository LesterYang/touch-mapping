#ifndef _TM_TEST_H
#define _TM_TEST_H

typedef struct _fb_data_t {
    int   num;
    char* dev;
    char* pan;
} fb_data_t;

typedef struct _evt_data_t {
    int   num;
    char* dev;
} evt_data_t;


int ts_test(fb_data_t* fb, evt_data_t* evt);
int open_framebuffer(fb_data_t* fb);
int open_slave_fb(fb_data_t* fb);
#endif
