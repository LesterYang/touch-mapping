/*
 * tm.c
 *
 *  Created on: Jul 15, 2014
 *      Author: root
 */
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "qUtils.h"
#include "tm.h"
#include "tmMap.h"
#include "tmInput.h"

struct sTmData* tm = NULL;

#define unlock_and_continue(mutex)      \
{                                       \
    q_mutex_unlock(mutex);              \
    continue;                           \
}

uint8_t g_header[Queue_HdrLen]={Queue_Hdr0, Queue_Hdr1};

#define QUEUE_DATA_SIZE (sizeof(g_header) + sizeof(sInputEv) + sizeof(struct sEventDev *) + sizeof(tm_op_code))

struct sStatusInfo{
    tm_main_status status;
    const char* str;
};

struct sStatusInfo statusInfo[] = {
    {eTmStatusNone,         "None"},
    {eTmStatusInputeInit,   "InputeInit"},
    {eTmStatusSocketInit,   "SocketInit"},
    {eTmStatusLoop,         "MainLoop"},
    {eTmStatusDeinit,       "Deinit"},
    {eTmStatusError,        "Error"},
    {eTmStatusExit,         "Exit"},
};


#if 1 // test on ubuntu

struct sEventDev srcEvDev[] = {
    { "/dev/input/event0",       -1,    eTmDevFront,    eTmApQSI,       NULL,   {NULL} },
    { "/dev/input/event1",       -1,    eTmDevFront,    eTmApMonitor,   NULL,   {NULL} },
    { "/dev/input/event2",       -1,    eTmDevFront,    eTmApNavi,      NULL,   {NULL} },
    { "/dev/input/mouse0",       -1,    eTmDevLeft,     eTmApQSILeft,   NULL,   {NULL} },
    { "/dev/input/mouse1",       -1,    eTmDevRight,    eTmApQSIRight,  NULL,   {NULL} },
    {                NULL,        0,    eTmDevNone,     eTmApNone,      NULL,   {NULL} }
};

struct sEventDev destEvDev[] = {
    { "/dev/null",       -1,    eTmDevFront,    eTmApQSI,      NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevFront,    eTmApMonitor,  NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevFront,    eTmApNavi,     NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevLeft,     eTmApQSILeft,  NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevRight,    eTmApQSIRight, NULL,   {NULL} },
    {        NULL,        0,    eTmDevNone,     eTmApNone,     NULL,   {NULL} }
};

#else

struct sEventDev srcEvDev[] = {
    { "/dev/input/event0",       -1,    eTmDevFront,    eTmApQSI,       NULL,   {NULL} },
    { "/dev/input/event1",       -1,    eTmDevFront,    eTmApMonitor,   NULL,   {NULL} },
    { "/dev/input/event2",       -1,    eTmDevFront,    eTmApNavi,      NULL,   {NULL} },
    { "/dev/input/event5",       -1,    eTmDevLeft,     eTmApQSILeft,   NULL,   {NULL} },
    { "/dev/input/event6",       -1,    eTmDevRight,    eTmApQSIRight,  NULL,   {NULL} },
    {                NULL,        0,    eTmDevNone,     eTmApNone,      NULL,   {NULL} }
};

struct sEventDev destEvDev[] = {
    { "/dev/input/event20",       -1,    eTmDevFront,    eTmApQSI,      NULL,   {NULL} },
    { "/dev/input/event21",       -1,    eTmDevFront,    eTmApMonitor,  NULL,   {NULL} },
    { "/dev/input/event22",       -1,    eTmDevFront,    eTmApNavi,     NULL,   {NULL} },
    { "/dev/input/event25",       -1,    eTmDevLeft,     eTmApQSILeft,  NULL,   {NULL} },
    { "/dev/input/event26",       -1,    eTmDevRight,    eTmApQSIRight, NULL,   {NULL} },
    {                 NULL,        0,    eTmDevNone,     eTmApNone,     NULL,   {NULL} }
};
#endif


void tm_switch_main_status(tm_main_status status)
{
    if(tm == NULL)
        return;

    q_dbg("status : %10s -> %s", q_strnull(statusInfo[tm->status].str), q_strnull(statusInfo[status].str));
    tm->status = status;

}

tm_main_status tm_get_main_status()
{
    if(tm == NULL)
        return eTmStatusInputeInit;
    return tm->status;
}

qerrno tm_init()
{
    int i, j;
    qerrno err_no;

    signal(SIGINT, tm_shutdown);
    signal(SIGHUP, tm_shutdown);

    if((err_no = tm_create(&tm)) != eENoErr)
    {
        q_dbg("tm_create : %s", tm_errorno_str(err_no));
        return err_no;
    }

    tm->status = eTmStatusNone;

    if(tm_inputInit(srcEvDev))
        return eEOpen;

    for(i=0; destEvDev[i].evDevPath != NULL; i++)
    {
        if((destEvDev[i].fd = open (destEvDev[i].evDevPath, O_RDONLY )) < 0)
            continue;

        q_dbg("Opened %s as target device [counter %d]",destEvDev[i].evDevPath, i);

        for(j=0; srcEvDev[j].evDevPath != NULL; j++)
        {
            if(srcEvDev[j].ap == destEvDev[i].ap)
            {
                srcEvDev[j].targetDev = &destEvDev[i];
                destEvDev[j].sourceDev = &srcEvDev[i];
            }
        }
    }

    tm_set_dev_param(srcEvDev, tm->panel);
    tm_set_dev_param(destEvDev, tm->fb);

    return eENoErr;
}

void tm_deinit()
{
    int idx;
    tm_destroy(tm);

    for(idx=0; destEvDev[idx].evDevPath != NULL; idx++)
    {
        if (destEvDev[idx].fd >= 0)
        {
            q_close(destEvDev[idx].fd);
            q_dbg("Closed target device [counter %d]", idx);
        }
    }

    tm_inputDeinit();
}

void tm_shutdown(int signum)
{
    tm_deinit();
    exit(signum);
}

void tm_set_dev_param(struct sEventDev* evDev, struct sTmDevParam* paramDev)
{
    int i;
    tm_dev dev;

    for(i=0; evDev[i].evDevPath != NULL; i++)
    {
        dev = evDev[i].dev;

        if( dev >= 0 && dev < eTmDevMax)
            evDev[i].paramDev = &paramDev[dev];
    }
}

void tm_set_direction(tm_dev source, tm_ap target_ap)
{
    int i, j;

    for(i=0; srcEvDev[i].evDevPath != NULL; i++)
    {
        if(srcEvDev[i].dev == source)
        {
            for(j=0; destEvDev[j].evDevPath != NULL; j++)
            {
                if(destEvDev[j].ap == target_ap)
                    break;
            }
            srcEvDev[i].targetDev = &destEvDev[j];
            destEvDev[j].sourceDev = &srcEvDev[i];
            q_dbg("set %d -> %d ",srcEvDev[i].ap, destEvDev[j].ap);
        }
    }
}

void tm_save_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op)
{
    q_dbg("%d -> type : %2x, code : %2x, value : %2x",srcEv->dev ,ev->type, ev->code, ev->value);

    if(tm->status != eTmStatusLoop)
        return;

    q_mutex_lock(tm->mutex);

    q_set_queue(tm->queue, g_header, sizeof(g_header), q_true);
    q_set_queue(tm->queue, ev, sizeof(sInputEv), q_true);
    q_set_queue(tm->queue, &srcEv, sizeof(struct sEventDev *), q_true);
    q_set_queue(tm->queue, &op, sizeof(tm_op_code), q_true);

   q_mutex_unlock(tm->mutex);

    return;
}

void tm_send_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op)
{
    qerrno err = eENoErr;
    tm_event_code code = eTmEventNone;
    int16_t value = 0;

    q_dbg("%d <- type : %2x, code : %2x, value : %2x",srcEv->dev ,ev->type, ev->code, ev->value);

    switch(op)
    {
        case eTmOpCodeTransX: code = eTmEventX; break;
        case eTmOpCodeTransY: code = eTmEventY; break;
        default:break;
    }

    if(code != eTmEventNone)
    {
        err = tm_transfer_value(&value, code, srcEv->paramDev, srcEv->targetDev->paramDev);

        if (err == eESwap)
        {
            if(ev->code == ABS_X)                   ev->code = ABS_Y;
            else if(ev->code == ABS_Y)              ev->code = ABS_X;
            else if(ev->code == ABS_MT_POSITION_X)  ev->code = ABS_MT_POSITION_Y;
            else if(ev->code == ABS_MT_POSITION_Y)  ev->code = ABS_MT_POSITION_X;
        }
        else if(err != eENoErr)
        {
            q_dbg("%s", tm_errorno_str(err));
            return;
        }

        ev->value = (int)value;
    }

    q_write(srcEv->targetDev->fd, ev, sizeof(sInputEv), NULL);

    return;
}

void tm_parse_event()
{
    char* buf;
    char hdr;
    tm_op_code op;
    struct sEventDev *srcEv;
    sInputEv ev;

    while(q_size_queue(tm->queue) >= QUEUE_DATA_SIZE)
    {
        q_mutex_lock(tm->mutex);

        if( q_pop_queue(tm->queue, &hdr) || hdr != (char)Queue_Hdr0 )
            unlock_and_continue(tm->mutex);

        if( q_pop_queue(tm->queue, &hdr) || hdr != (char)Queue_Hdr1 )
            unlock_and_continue(tm->mutex);

        buf = (char*)&ev;
        if(q_get_queue(tm->queue, buf, sizeof(sInputEv)) != sizeof(sInputEv))
            unlock_and_continue(tm->mutex);

        buf = (char*)&srcEv;
        if(q_get_queue(tm->queue, buf, sizeof(struct sEventDev *)) != sizeof(struct sEventDev *))
            unlock_and_continue(tm->mutex);

        buf = (char*)&op;
        if(q_get_queue(tm->queue, buf, sizeof(tm_op_code)) != sizeof(tm_op_code))
            unlock_and_continue(tm->mutex);

        q_mutex_unlock(tm->mutex);
        tm_send_event(&ev, srcEv, op);
    }



}
