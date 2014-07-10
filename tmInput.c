/*
 * qInput.c
 *
 *  Created on: Jul 9, 2014
 *      Author: lester
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <linux/input.h>

#include "qUtils.h"
#include "tmMap.h"
#include "tmInput.h"


#define mark_input 0


#if 1 // test on ubuntu

struct sInputEvDev srcEvDev[] = {
    { "/dev/input/event0",       -1},
    { "/dev/input/event1",       -1},
    { "/dev/input/event2",       -1},
    { "/dev/input/mouse0",       -1},
    { "/dev/input/mouse1",       -1},
    {      NULL,      0}
};

struct sInputEvDev destEvDev[] = {
    {      NULL,      0},
    {      NULL,      0}
};

#else
struct sInputEvDev srcEvDev[] = {
    { "/dev/input/event0",       -1},
    { "/dev/input/event1",       -1},
    { "/dev/input/event2",       -1},
    { "/dev/input/event3",       -1},
    {      NULL,      0}
};

struct sInputEvDev destEvDev[] = {
    { "/dev/input/event20",       -1},
    { "/dev/input/event21",       -1},
    { "/dev/input/event22",       -1},
    { "/dev/input/event23",       -1},
    {      NULL,      0}
};
#endif

typedef struct sInputData {
    fd_set skfds;
    fd_set  evfds;
    int maxfd;

    q_thread  *thread;

    struct {
        volatile char openInput     :1;
        char reserved               :7;
    }flag;

}sInputData;

sInputData qInputData;


int  tm_inputInitEvents();
void tm_inputCloseEvents();
void tm_inputCleanStdin();
int  tm_inputAddFd (fd_set * fdsp);
int  tm_inputParseDev();
static void tm_inputThread(void *data);

int tm_inputInit()
{
#if mark_input
    return 0;
#endif

    memset(&qInputData, 0, sizeof(qInputData));

    if ( tm_inputInitEvents() < 1 )
    {
        qerror("Failed to open event interface files");
        return 1;
    }

    qInputData.maxfd = tm_inputAddFd( &qInputData.evfds );

    if ( qInputData.maxfd <= 0 )
    {
        qerror("Failed to organize event input");
    }

    qInputData.thread = q_thread_new(tm_inputThread, NULL);
    return 0;
}

void tm_inputDeinit()
{
#if mark_input
    return;
#endif

    tm_inputCloseEvents();

    if (qInputData.thread){
        q_thread_free(qInputData.thread);
        qInputData.thread = NULL;
    }
}


int tm_inputInitEvents()
{
    int  idx;

    tm_inputCleanStdin();

    for(idx=0; destEvDev[idx].evDevName != NULL; idx++)
    {
        if((destEvDev[idx].fd = open (destEvDev[idx].evDevName, O_RDONLY )) < 0)
            continue;

        q_dbg("Opened %s as target device [counter %d]",destEvDev[idx].evDevName, idx);
    }

    for(idx=0; srcEvDev[idx].evDevName != NULL; idx++)
    {
        if((srcEvDev[idx].fd = open (srcEvDev[idx].evDevName, O_RDONLY )) < 0)
            continue;

        q_dbg("Opened %s as event device [counter %d]",srcEvDev[idx].evDevName, idx);
    }

    if (idx > 0)
        qInputData.flag.openInput = q_true;

    return idx;
}

void tm_inputCloseEvents()
{
    int idx;

    for(idx=0; destEvDev[idx].evDevName != NULL; idx++)
    {
        if (destEvDev[idx].fd >= 0)
        {
            q_close(destEvDev[idx].fd);
            q_dbg("Closed target device [counter %d]", idx);
        }
    }

    for(idx=0; srcEvDev[idx].evDevName != NULL; idx++)
    {
        if (srcEvDev[idx].fd >= 0)
        {
            q_close(srcEvDev[idx].fd);
            q_dbg("Closed event device [counter %d]", idx);
        }
    }

    qInputData.flag.openInput = q_false;
    return;
}

void tm_inputCleanStdin()
{
    fd_set fds;
    struct timeval tv;
    char buf[8];
    int stdin = 0, stdout = 1;

    FD_ZERO (&fds);
    FD_SET (stdin, &fds);
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    while (0 < select(stdout, &fds, NULL, NULL, &tv))
    {
        while (q_read(stdin, buf, 8)) {;}
        FD_ZERO (&fds);
        FD_SET (stdin, &fds);
        tv.tv_sec  = 0;
        tv.tv_usec = 1;
    }
    q_close(stdin);
    return;
}

int tm_inputAddFd(fd_set * fdsp)
{
    int idx, maxfd;
    FD_ZERO(fdsp);

    maxfd = -1;

    for(idx=0; srcEvDev[idx].evDevName != NULL; idx++)
    {
        if (srcEvDev[idx].fd >= 0)
        {
            FD_SET(srcEvDev[idx].fd, fdsp);
            if (srcEvDev[idx].fd > maxfd)
                maxfd = srcEvDev[idx].fd;
        }
    }

    return maxfd;
}

int tm_inputParseDev()
{
    q_bool trans = q_false;
    int i, j;
    char buf[sizeof(struct input_event)] = {0};
    struct input_event *inEvent = (struct input_event *)buf;

    if (&qInputData.evfds == NULL)
        return -1;

    for(i=0; srcEvDev[i].evDevName != NULL; i++)
    {
        if (srcEvDev[i].fd < 0)
            continue;

        if (!(FD_ISSET(srcEvDev[i].fd, &qInputData.evfds)))
            continue;

        j = q_loop_read(srcEvDev[i].fd, buf, sizeof(struct input_event));

        if(j == (int)sizeof(struct input_event))
            break;
        /*
        if ( (j == 0) || (j == -1) || ((int)sizeof(struct input_event) > j) )
            continue;
        else
            break;
            */
    }

    q_dbg("dev %d => type : %2x, code : %2x, value : %2x",i ,inEvent->type, inEvent->code, inEvent->value);

    switch (inEvent->type)
    {
        case EV_SYN:
            break;
        case EV_KEY:
            switch (inEvent->code)
            {
                case BTN_TOUCH:
                    break;
                default:
                    break;
            }
            break;

        case EV_ABS:
            switch (inEvent->code)
            {
                case ABS_X:
                case ABS_Y:
                case ABS_MT_POSITION_X:
                case ABS_MT_POSITION_Y:
                    trans = q_true;
                    break;
            }
        default:
            break;
    }

    tm_send_event(inEvent, destEvDev, trans);
    return 0;
}

static void tm_inputThread(void *data)
{
    struct timeval tv;

    while (qInputData.flag.openInput)
    {
        tm_inputAddFd ( &qInputData.evfds );
        tv.tv_sec  = 0;
        tv.tv_usec = 50000;

        while ( 0 < select((qInputData.maxfd)+1, &qInputData.evfds, NULL, NULL, &tv) )
        {
            if ( 0 > tm_inputParseDev() )
            {
                fprintf(stderr, "IO thread failed\n");
                break;
            }

            if(qInputData.flag.openInput == q_false)
                break;

            tm_inputAddFd(&qInputData.evfds);
            tv.tv_sec  = 0;
            tv.tv_usec = 500000;
        }
    }
}
