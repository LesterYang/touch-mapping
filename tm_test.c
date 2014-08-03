/*
 * tm_test.c
 *
 *  Created on: Aug 1, 2014
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <linux/input.h>
#include "tm.h"
#include "qUtils.h"
#include "tmMapping.h"
#include "tmInput.h"

void tm_test_transfer_cal()
{
    //ev_test 421 695, 521 882
    //ts_test 473 115, 384 0
    tm_ap_info_t* ap;
    int x=521,y=882;
    q_dbg("send %d %d \n",x,y);
    ap = tm_transfer(&x, &y, NULL);
    if(ap)
        q_dbg("send to %s",ap->evt_path);
    q_dbg("get %d %d (ans: 384 0)\n",x,y);
}

void tm_event(int x, int y)
{
    int fd=0;
    struct input_event absX={
        .type=EV_ABS,
        .code=ABS_X,
    };
    struct input_event absY={
        .type=EV_ABS,
        .code=ABS_Y,
    };
    struct input_event absP={
        .type=EV_ABS,
        .code=ABS_MT_TOUCH_MAJOR,
        .value=250
    };
    struct input_event sync={
        .type=EV_SYN,
        .code=SYN_REPORT,
        .value=0
    };

#if 0
    struct input_event mt_sync={
        .type=EV_SYN,
        .code=SYN_MT_REPORT,
        .value=0
    };
#endif
    absX.code=x;
    gettimeofday(&absX.time, NULL);
    if(write(fd,&absX,sizeof(struct input_event)) == -1)
        printf("write error\n");

    absY.code=y;
    gettimeofday(&absY.time, NULL);
    if(write(fd,&absY,sizeof(struct input_event)) == -1)
        printf("write error\n");

    gettimeofday(&absP.time, NULL);
    if(write(fd,&absP,sizeof(struct input_event)) == -1)
        printf("write error\n");
#if 0
    gettimeofday(&mt_sync.time, NULL);
    if(write(fd,&mt_sync,sizeof(struct input_event)) == -1)
        printf("write error\n");
#endif
    gettimeofday(&sync.time, NULL);
    if(write(fd,&sync,sizeof(struct input_event)) == -1)
        printf("write error\n");
}

void tm_test_event()
{

}

#if 0//check frame buffer mapping
#define test_src_x 800
#define test_src_y 480
#define test_dest_x 1000
#define test_dest_y 600
tm_fb_param_t test_src_param[] = {
    {0, test_src_x, test_src_y, 0, 0, 0}, //lt
    {1, test_src_x, test_src_y, 0, 0, 1},
    {2, test_src_x, test_src_y, 1, 0, 0}, //rt
    {3, test_src_x, test_src_y, 1, 0, 1},
    {4, test_src_x, test_src_y, 1, 1, 0}, //rb
    {5, test_src_x, test_src_y, 1, 1, 1},
    {6, test_src_x, test_src_y, 0, 1, 0}, //lb
    {7, test_src_x, test_src_y, 0, 1, 1}
};

tm_fb_param_t test_dest_param[] = {
    {0, test_dest_x, test_dest_y, 0, 0, 0},
    {0, test_dest_x, test_dest_y, 0, 0, 1},
    {0, test_dest_x, test_dest_y, 1, 0, 0},
    {0, test_dest_x, test_dest_y, 1, 0, 1},
    {0, test_dest_x, test_dest_y, 1, 1, 0},
    {0, test_dest_x, test_dest_y, 1, 1, 1},
    {0, test_dest_x, test_dest_y, 0, 1, 0},
    {0, test_dest_x, test_dest_y, 0, 1, 1},
};

tm_fb_param_t src_point[] = {
    {0, 300, 120, 0, 0, 0},
    {0, 120, 300, 0, 0, 0},
    {0, 500, 120, 0, 0, 0},
    {0, 120, 500, 0, 0, 0},
    {0, 500, 360, 0, 0, 0},
    {0, 360, 500, 0, 0, 0},
    {0, 300, 360, 0, 0, 0},
    {0, 360, 300, 0, 0, 0},
};

tm_fb_param_t ans[] = {
    {0, 375, 150, 0, 0, 0},
    {0, 150, 375, 0, 0, 0},
    {0, 625, 150, 0, 0, 0},
    {0, 150, 625, 0, 0, 0},
    {0, 625, 450, 0, 0, 0},
    {0, 450, 625, 0, 0, 0},
    {0, 375, 450, 0, 0, 0},
    {0, 450, 375, 0, 0, 0},
};

void tm_mapping_test()
{
    int i,j;
    tm_config_t conf;
    tm_config_t* config = &conf;
    tm_fb_param_t* src_fb;
    tm_fb_param_t* dest_fb;
    static union {
        int vec[3];
        struct{
            int x;
            int y;
            int z;
        };
    }coord;

    conf.scaling = 1;

    for(i=0;i<8;i++)
    {
        coord.x = src_point[i].max_x;
        coord.y = src_point[i].max_y;
        coord.z = 1;

        printf("%d : %d %d\n",i, coord.x,coord.y);
        src_fb = &test_src_param[i];

        for(j=0;j<8;j++)
        {
            coord.x = src_point[i].max_x;
            coord.y = src_point[i].max_y;
            coord.z = 1;

            dest_fb = &test_dest_param[j];

            if (src_fb == dest_fb)
            {
                continue;
            }

            if(src_fb->swap)
            {
                coord.z = coord.x;
                coord.x = coord.y;
                coord.y = coord.z;
                coord.z = 1;
            }

            if(src_fb->horizontal != dest_fb->horizontal)
                coord.x = src_fb->max_x - coord.x;

            if(src_fb->vertical != dest_fb->vertical)
                coord.y = src_fb->max_y - coord.y;

            coord.x = (dest_fb->max_x * coord.x) / (src_fb->max_x * config->scaling);
            coord.y = (dest_fb->max_y * coord.y) / (src_fb->max_y * config->scaling);

            if(dest_fb->swap)
            {
                coord.z = coord.x;
                coord.x = coord.y;
                coord.y = coord.z;
                coord.z = 1;
            }

            if(coord.x == ans[j].max_x && coord.y == ans[j].max_y)
                printf("ok   -> %d : %d %d\n",j, coord.x,coord.y);
            else
                printf("fail -> %d : %d %d (ans : %d %d)\n",j, coord.x,coord.y,ans[j].max_x,ans[j].max_y);
        }
    }
}
#endif


void tm_mapping_test()
{

}

void tm_test()
{
    tm_test_transfer_cal();
    tm_mapping_test();
    tm_test_event();
}
