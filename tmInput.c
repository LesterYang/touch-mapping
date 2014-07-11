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
struct sEventDev *srcEv;

int  tm_inputInitEvents();
void tm_inputCloseEvents();
void tm_inputCleanStdin();
int  tm_inputAddFd (fd_set * fdsp);
int  tm_inputParseDev();
static void tm_inputThread(void *data);

int tm_inputInit(struct sEventDev* evDev)
{
#if mark_input
    return 0;
#endif
    q_assert(evDev);

    srcEv = evDev;
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

    for(idx=0; srcEv[idx].evDevPath != NULL; idx++)
    {
        if((srcEv[idx].fd = open (srcEv[idx].evDevPath, O_RDONLY )) < 0)
            continue;

        q_dbg("Opened %s as event device [counter %d]",srcEv[idx].evDevPath, idx);
    }

    if (idx > 0)
        qInputData.flag.openInput = q_true;

    return idx;
}

void tm_inputCloseEvents()
{
    int idx;

    for(idx=0; srcEv[idx].evDevPath != NULL; idx++)
    {
        if (srcEv[idx].fd >= 0)
        {
            q_close(srcEv[idx].fd);
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

    for(idx=0; srcEv[idx].evDevPath != NULL; idx++)
    {
        if (srcEv[idx].fd >= 0)
        {
            FD_SET(srcEv[idx].fd, fdsp);
            if (srcEv[idx].fd > maxfd)
                maxfd = srcEv[idx].fd;
        }
    }

    return maxfd;
}

int tm_inputParseDev()
{
    tm_op_code op = eTmOpCodeNone;
    struct sEventDev *src_dev= NULL;
    int i, j;
    char buf[sizeof(sInputEv)] = {0};
    sInputEv *inEvent = (sInputEv *)buf;

    if (&qInputData.evfds == NULL)
        return -1;

    for(i=0; srcEv[i].evDevPath != NULL; i++)
    {
        if (srcEv[i].fd < 0)
            continue;

        if (!(FD_ISSET(srcEv[i].fd, &qInputData.evfds)))
            continue;

        if(qInputData.flag.openInput == q_false)
            return 0;

        j = q_loop_read(srcEv[i].fd, buf, sizeof(sInputEv));

        if(j == (int)sizeof(sInputEv))
            break;
    }

    src_dev = &srcEv[i];

    switch (inEvent->type)
    {
        case EV_SYN:
            op = eTmOpCodeEnd;
            break;
        case EV_KEY:
            if(inEvent->code == BTN_TOUCH && inEvent->value)
                op = eTmOpCodeStart;
            break;

        case EV_ABS:
            switch (inEvent->code)
            {
                case ABS_X:
                case ABS_Y:
                case ABS_MT_POSITION_X:
                case ABS_MT_POSITION_Y:
                    op = eTmOpCodeTrans;
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    tm_save_event(inEvent, src_dev, op);
    return 0;
}

static void tm_inputThread(void *data)
{
    struct timeval tv;
    int ret;

    while(qInputData.flag.openInput)
    {
        tm_inputAddFd( &qInputData.evfds );
        tv.tv_sec  = 0;
        tv.tv_usec = 50000;

        while( (ret = select((qInputData.maxfd)+1, &qInputData.evfds, NULL, NULL, &tv)) >= 0)
        {
            if( ret > 0 && tm_inputParseDev() < 0)
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
