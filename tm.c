/*
 * tm.c
 *
 *  Created on: Jul 15, 2014
 *      Author: root
 */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "tm.h"
#include "qUtils.h"
#include "tmMapping.h"
#include "tmInput.h"

#if 1

typedef struct _tm_info
{
    q_mutex*            mutex;
    tm_status_t*        status;
    tm_event_info_t*    event;
    tm_panel_info_t*    panel;
}tm_info_t;

tm_info_t tm;

tm_event_info_t g_event[] = {
    {TM_AP_QSI,     "/dev/input/event0", -1, "/dev/input/event20", -1, NULL},
    {TM_AP_QSI_L,   "/dev/input/event1", -1, "/dev/input/event21", -1, NULL},
    {TM_AP_QSI_R,   "/dev/input/event2", -1, "/dev/input/event22", -1, NULL},
    {TM_AP_NAVI,    "/dev/input/event3", -1, "/dev/input/event23", -1, NULL},
    {TM_AP_MONITOR, "/dev/input/event4", -1, "/dev/input/event24", -1, NULL},
    {TM_AP_NONE,                   NULL,  0,                 NULL,   0, NULL}
};

tm_panel_info_t g_panel[] = {
    {TM_PANEL_FRONT, NULL, NULL, NULL},
    {TM_PANEL_LEFT,  NULL, NULL, NULL},
    {TM_PANEL_RIGHT, NULL, NULL, NULL},
    {TM_PANEL_NONE,  NULL, NULL, NULL}
};



const char* tm_err_str(tm_errno_t no)
{
    switch(no)
    {
        case TM_ERRNO_SUCCESS:      return " No error";
        case TM_ERRNO_NO_DEV:       return " No such device";
        case TM_ERRNO_DEV_PARAM:    return " Parameter of device error";
        case TM_ERRNO_DEV_NUM:      return " Bad device number";
        case TM_ERRNO_ALLOC:        return " Allocate error";
        case TM_ERRNO_OPEN:         return " Open file error";
        case TM_ERRNO_POINT:        return " Points are out of rage";
        case TM_ERRNO_PARAM:        return " Function parameter error";
        case TM_ERRNO_SWAP:         return " Need to swap xy";
        default:            break;
    }
    return "unknown";
}

void tm_set_default()
{

}



tm_errno_t tm_init()
{
    tm_errno_t err_no;

    tm.event = g_event;
    tm.panel = g_panel;
    tm.mutex = q_mutex_new(q_true, q_true);

    if((err_no = tm_mapping_create_handler()) != TM_ERRNO_SUCCESS)
    {
        q_dbg("tm_create : %s", tm_err_str(err_no));
        return err_no;
    }

    if((err_no = tm_input_init(tm.panel, tm.event)) != TM_ERRNO_SUCCESS)
    {
        q_dbg("tm_create : %s", tm_err_str(err_no));
        return err_no;
    }

    // set default direction
    tm.panel[TM_PANEL_FRONT].dest_panel = &tm.panel[TM_PANEL_FRONT];
    tm.panel[TM_PANEL_FRONT].current    = &tm.event[TM_AP_QSI];
    tm.panel[TM_PANEL_LEFT].dest_panel  = &tm.panel[TM_PANEL_LEFT];
    tm.panel[TM_PANEL_LEFT].current     = &tm.event[TM_AP_QSI_L];
    tm.panel[TM_PANEL_RIGHT].dest_panel = &tm.panel[TM_PANEL_RIGHT];
    tm.panel[TM_PANEL_RIGHT].current    = &tm.event[TM_AP_QSI_R];


    return TM_ERRNO_SUCCESS;
}

void tm_deinit()
{
    tm_mapping_destroy_handler();
    tm_input_deinit();
}

void tm_bind_panel_ap(tm_panel_t panel, tm_panel_t ap)
{
    tm.panel[panel].current = &tm.event[ap];
}

void tm_set_direction(tm_panel_t source, tm_panel_t target)
{
    tm.panel[source].dest_panel = &tm.panel[target];
}

void tm_set_status(tm_status_t status)
{
    q_mutex_lock(tm.mutex);
    *tm.status = status;
    q_mutex_unlock(tm.mutex);
}

void tm_bind_status(tm_status_t* status)
{
    tm.status = status;
}


tm_errno_t tm_transfer(int16_t *x, int16_t *y, tm_panel_info_t* panel)
{
    return tm_mapping_transfer(x, y, panel->param, panel->current->fb_param, panel->dest_panel->current->fb_param);
}

#else



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
#endif
