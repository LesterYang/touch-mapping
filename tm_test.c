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

typedef struct _tm_input_dev tm_input_dev_t;
void tm_input_parse_single_touch(tm_input_dev_t* dev, tm_input_event_t* evt);
void tm_input_parse_mt_A(tm_input_dev_t* dev, tm_input_event_t* evt);

void tm_test_transfer_cal()
{
    //ev_test 421 695, 521 882
    //ts_test 473 115, 384 0
    tm_ap_info_t* ap;
    int x=521,y=882;
    q_dbg(Q_DBG_MAP, "send %d %d \n",x,y);
    ap = tm_transfer(&x, &y, NULL);
    if(ap)
        q_dbg(Q_DBG_MAP, "send to %s",ap->evt_path);
    q_dbg(Q_DBG_MAP, "get %d %d (ans: 384 0)\n",x,y);
}

void tm_event_press()
{
    tm_input_event_t touch={
        .type=EV_KEY,
        .code=BTN_TOUCH,
        .value=1
    };
    tm_input_event_t sync={
        .type=EV_SYN,
        .code=SYN_REPORT,
        .value=0
    };
    gettimeofday(&touch.time, NULL);
    gettimeofday(&sync.time, NULL);
    tm_input_parse_single_touch(NULL, &touch);
    tm_input_parse_single_touch(NULL, &sync);
}

void tm_event_release()
{
    tm_input_event_t touch={
        .type=EV_KEY,
        .code=BTN_TOUCH,
        .value=0
    };
    tm_input_event_t absP={
        .type=EV_ABS,
        .code=ABS_PRESSURE,
        .value=0
    };
    tm_input_event_t sync={
        .type=EV_SYN,
        .code=SYN_REPORT,
        .value=0
    };
    gettimeofday(&touch.time, NULL);
    gettimeofday(&absP.time, NULL);
    gettimeofday(&sync.time, NULL);
    tm_input_parse_single_touch(NULL, &touch);
    tm_input_parse_single_touch(NULL, &absP);
    tm_input_parse_single_touch(NULL, &sync);
}

void tm_event(int x, int y)
{
    tm_input_event_t absX={
        .type=EV_ABS,
        .code=ABS_X,
    };
    tm_input_event_t absY={
        .type=EV_ABS,
        .code=ABS_Y,
    };
    tm_input_event_t absP={
        .type=EV_ABS,
        .code=ABS_PRESSURE,
        .value=250
    };
    tm_input_event_t sync={
        .type=EV_SYN,
        .code=SYN_REPORT,
        .value=0
    };

    absX.value=x;
    gettimeofday(&absX.time, NULL);
    absY.value=y;
    gettimeofday(&absY.time, NULL);
    gettimeofday(&absP.time, NULL);
    gettimeofday(&sync.time, NULL);

    tm_input_parse_single_touch(NULL, &absX);
    tm_input_parse_single_touch(NULL, &absY);
    tm_input_parse_single_touch(NULL, &absP);
    tm_input_parse_single_touch(NULL, &sync);
}

void tm_event_release_mt_A()
{
    tm_input_event_t absP={
        .type=EV_ABS,
        .code=ABS_MT_TOUCH_MAJOR,
        .value=0
    };
    tm_input_event_t mt_sync={
        .type=EV_SYN,
        .code=SYN_MT_REPORT,
        .value=0
    };

    gettimeofday(&absP.time, NULL);
    gettimeofday(&mt_sync.time, NULL);

    tm_input_parse_mt_A(NULL, &absP);
    tm_input_parse_mt_A(NULL, &mt_sync);

}

void tm_event_sync_mt_A()
{
    tm_input_event_t sync={
        .type=EV_SYN,
        .code=SYN_REPORT,
        .value=0
    };
    gettimeofday(&sync.time, NULL);
    tm_input_parse_mt_A(NULL, &sync);
}

void tm_event_mt_A(int x, int y)
{
    tm_input_event_t absX={
        .type=EV_ABS,
        .code=ABS_MT_POSITION_X,
    };
    tm_input_event_t absY={
        .type=EV_ABS,
        .code=ABS_MT_POSITION_Y,
    };
    tm_input_event_t absP={
        .type=EV_ABS,
        .code=ABS_MT_TOUCH_MAJOR,
        .value=250
    };
    tm_input_event_t mt_sync={
        .type=EV_SYN,
        .code=SYN_MT_REPORT,
        .value=0
    };

    absX.value=x;
    gettimeofday(&absX.time, NULL);
    absY.value=y;
    gettimeofday(&absY.time, NULL);
    gettimeofday(&absP.time, NULL);
    gettimeofday(&mt_sync.time, NULL);

    tm_input_parse_mt_A(NULL, &absX);
    tm_input_parse_mt_A(NULL, &absY);
    tm_input_parse_mt_A(NULL, &absP);
    tm_input_parse_mt_A(NULL, &mt_sync);
}

void tm_test_event()
{
// single touch
#if 1
    q_dbg(Q_DBG_POINT,"test single touch !!!!!");
    tm_event_press();
    tm_event(421,695);
    tm_event(421,695);
    tm_event(421,695);
    tm_event(521,882);
    tm_event(491,625);
    tm_event_release();
    tm_event_press();
    tm_event(421,695);
    tm_event(200,300);
    tm_event_release();
#endif
// mt_A
#if 0
    q_dbg(Q_DBG_POINT,"test mt A !!!!!");
    tm_event_mt_A(421,695);
    tm_event_mt_A(521,882);
    tm_event_sync_mt_A();
    tm_event_mt_A(521,882);
    tm_event_sync_mt_A();
    tm_event_release_mt_A();
    tm_event_sync_mt_A();
    tm_event_mt_A(200,300);
    tm_event_sync_mt_A();
    tm_event_release_mt_A();
    tm_event_sync_mt_A();
#endif
// mt_B
#if 1

#endif
}

void tm_mapping_test()
{

}

void tm_redirct_test()
{
    // panel,start_x,start_y,per_width,per_high,ap,start_x,start_y,per_width,per_high
    unsigned char msg_p1a0[]={1, 50, 0, 50, 100, 0, 0, 0, 100, 100};
    unsigned char msg_p1a1[]={1,  0, 0, 50, 100, 1, 0, 0, 100, 100};

    unsigned char msg_p0a0[]={0,  0,  0, 50, 50, 0,  0,  0, 100, 100};
    unsigned char msg_p0a1[]={0, 50,  0, 50, 50, 1,  0,  0, 100, 100};
    unsigned char msg_p0a2[]={0, 50, 50, 50, 50, 2, 25, 25,  50,  50};
    unsigned char msg_p0a3[]={0,  0, 50, 50, 50, 3,  0,  0, 100, 100};

    tm_dbg_print_conf();

    tm_set_map(sizeof(msg_p1a0), msg_p1a0, q_false);
    tm_set_map(sizeof(msg_p1a1), msg_p1a1, q_true);
    tm_set_map(sizeof(msg_p0a0), msg_p0a0, q_false);
    tm_set_map(sizeof(msg_p0a1), msg_p0a1, q_true);
    tm_set_map(sizeof(msg_p0a2), msg_p0a2, q_true);
    tm_set_map(sizeof(msg_p0a3), msg_p0a3, q_true);

    tm_dbg_print_conf();
}

void tm_test()
{
    tm_test_transfer_cal();
    tm_mapping_test();
    tm_redirct_test();
    tm_test_event();
}
