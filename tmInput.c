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

#include "tmInput.h"
#include "qUtils.h"
#include "tm.h"
#include "tmMapping.h"

#if 1

typedef struct _tm_input_dev {
    tm_panel_info_t*  panel;
    q_thread*         thread;
    int               maxfd;
    fd_set            evfds;
    tm_input_status_t status;
}tm_input_dev_t;

typedef struct _tm_input_handler {
    q_bool             open;
    tm_panel_info_t*   panel;
    tm_ap_info_t*      ap;
    tm_input_dev_t*    dev;
}tm_input_handler_t;

static tm_input_handler_t tm_input;
static tm_input_dev_t     tm_input_dev[TM_PANEL_NUM];

void tm_send_event(tm_input_dev_t* dev, tm_input_event_t* event); // Global array in tmInput.c, tm_input_handler_t tm_input[TM_PANEL_NUM]
void tm_input_parse_event(tm_input_dev_t* dev);

void tm_input_clean_stdin();
int  tm_input_init_events();
void tm_input_close_events();

int  tm_input_add_fd (tm_panel_info_t* tm_input, fd_set * fdsp);

void tm_input_thread_func(void *data);

void tm_input_clean_stdin()
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

tm_errno_t tm_input_init(tm_panel_info_t* panel, tm_ap_info_t* ap)
{
    int idx;
    tm_errno_t err_no;
    q_assert(panel);
    q_assert(ap);

    tm_input.panel  = panel;
    tm_input.ap     = ap;
    tm_input.dev    = tm_input_dev;

    if((err_no = tm_input_init_events()) != TM_ERRNO_SUCCESS)
        return err_no;

    tm_input.open = q_true;

    for(idx=0; idx<TM_PANEL_NUM; idx++)
    {
        tm_input.dev[idx].panel  = &tm_input.panel[idx];
        tm_input.dev[idx].status = TM_INPUT_STATUS_NONE;
        tm_input.dev[idx].thread = q_thread_new(tm_input_thread_func, &tm_input.dev[idx]);
    }

    return TM_ERRNO_SUCCESS;
}

void tm_input_deinit()
{
    int idx;

    tm_input.open = q_false;

    tm_input_close_events();

    for(idx=0; tm_input.panel[idx].name != TM_PANEL_NONE; idx++)
    {
        if(tm_input.dev[idx].thread)
        {
            q_thread_free(tm_input.dev[idx].thread);
            tm_input.dev[idx].thread = NULL;
        }
    }
}

tm_errno_t  tm_input_init_events()
{
    int idx;

    tm_input_clean_stdin();

    for(idx=0; tm_input.panel[idx].name != TM_PANEL_NONE; idx++)
    {
        if((tm_input.panel[idx].fd = open (tm_input.panel[idx].event_path, O_RDONLY )) < 0)
            q_dbg("Opened %s error",tm_input.panel[idx].event_path);
    }

    for(idx=0; tm_input.ap[idx].name != TM_AP_NONE; idx++)
    {
        if((tm_input.ap[idx].fd = open (tm_input.ap[idx].event_path, O_RDONLY )) < 0)
            q_dbg("Opened %s error",tm_input.ap[idx].event_path);
    }

    return TM_ERRNO_SUCCESS;
}

void tm_input_close_events()
{
    int idx;

    for(idx=0; tm_input.panel[idx].name != TM_PANEL_NONE; idx++)
    {
        if(tm_input.panel[idx].fd > 0)
            q_close(tm_input.panel[idx].fd);
    }

    for(idx=0; tm_input.ap[idx].name != TM_AP_NONE; idx++)
    {
        if(tm_input.ap[idx].fd > 0)
            q_close(tm_input.ap[idx].fd);
    }
}

void tm_send_event(tm_input_dev_t* dev, tm_input_event_t* event)
{
    q_dbg("%s, code:%3d value:%3d",dev->panel->event_path, event->code, event->value);

    switch(dev->status)
    {
        case TM_INPUT_STATUS_END:break;
        case TM_INPUT_STATUS_TRANS:break;
        case TM_INPUT_STATUS_COLLECT:break;
        default:break;
    }
}

void tm_input_parse_event(tm_input_dev_t* dev)
{
    char buf[sizeof(tm_input_event_t)] = {0};
    tm_input_event_t* event = (tm_input_event_t *)buf;

    if ((dev->panel->fd < 0) || !(FD_ISSET(dev->panel->fd, &dev->evfds)))
        return;

    if(tm_input.open == q_false)
        return;

    if(q_loop_read(dev->panel->fd, buf, sizeof(tm_input_event_t)) != (ssize_t)sizeof(tm_input_event_t))
        return;

    switch (event->type)
    {
        case EV_SYN:
            dev->status = TM_INPUT_STATUS_END;
            break;
        case EV_KEY:
            if(event->code == BTN_TOUCH && event->value)
                dev->status = TM_INPUT_STATUS_START;
            break;

        case EV_ABS:
            switch (event->code)
            {
                case ABS_X:
                case ABS_Y:
                    if(dev->status == TM_INPUT_STATUS_START)
                        dev->status = TM_INPUT_STATUS_COLLECT;
                    else if(dev->status == TM_INPUT_STATUS_COLLECT)
                        dev->status = TM_INPUT_STATUS_TRANS;
                    break;
                case ABS_MT_POSITION_X:
                case ABS_MT_POSITION_Y:
                    if(dev->status == TM_INPUT_STATUS_START)
                        dev->status = TM_INPUT_STATUS_COLLECT;
                    else if(dev->status == TM_INPUT_STATUS_COLLECT)
                        dev->status = TM_INPUT_STATUS_TRANS;
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    tm_send_event(dev, event);
}

void tm_input_thread_func(void *data)
{
    struct timeval tv;
    int ret;
    tm_input_dev_t* dev = (tm_input_dev_t*)data;

    q_dbg("thread run : name : %d %s",dev->panel->name, dev->panel->event_path);

    FD_ZERO(&dev->evfds);
    FD_SET(dev->panel->fd, &dev->evfds);

    dev->maxfd = dev->panel->fd;

    while(tm_input.open)
    {
        FD_ZERO(&dev->evfds);
        FD_SET(dev->panel->fd, &dev->evfds);
        tv.tv_sec  = 0;
        tv.tv_usec = 50000;

        while( (ret = select((dev->maxfd)+1,  &dev->evfds, NULL, NULL, &tv)) >= 0)
        {
            if(tm_input.open == q_false)
                break;

            if(ret > 0)
            {
                tm_input_parse_event(dev);
            }

            FD_ZERO(&dev->evfds);
            FD_SET(dev->panel->fd, &dev->evfds);
            tv.tv_sec  = 0;
            tv.tv_usec = 500000;
        }
    }
}

#else

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
        return 1;
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

    fprintf(stderr, "set FD\n");

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
                case ABS_MT_POSITION_X:
                    op = eTmOpCodeTransX;
                    break;
                case ABS_Y:
                case ABS_MT_POSITION_Y:
                    op = eTmOpCodeTransY;
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
            if(ret > 0)
            {
                tm_inputParseDev();
            }

            if(qInputData.flag.openInput == q_false)
                break;

            tm_inputAddFd(&qInputData.evfds);
            tv.tv_sec  = 0;
            tv.tv_usec = 500000;
        }
    }
}
#endif
