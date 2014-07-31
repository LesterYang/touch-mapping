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
    tm_ap_info_t*       ap;
    tm_panel_info_t*    panel;

    tm_display_t*       dis_conf;
    tm_ipc_status_t     flag;
}tm_info_t;

static tm_info_t tm;


tm_ap_info_t g_ap[] = {
    {TM_AP_QSI,     "/dev/input/event20", -1, NULL, NULL},
    {TM_AP_QSI_L,   "/dev/input/event21", -1, NULL, NULL},
    {TM_AP_QSI_R,   "/dev/input/event22", -1, NULL, NULL},
    {TM_AP_NAVI,    "/dev/input/event23", -1, NULL, NULL},
    {TM_AP_MONITOR, "/dev/input/event24", -1, NULL, NULL},
    {TM_AP_NONE,                    NULL,  0, NULL}
};

tm_panel_info_t g_panel[] = {
    {TM_PANEL_FRONT, "/dev/input/event0", -1, 0, {0}, NULL, NULL, NULL},
    {TM_PANEL_LEFT,  "/dev/input/event1", -1, 0, {0}, NULL, NULL, NULL},
    {TM_PANEL_RIGHT, "/dev/input/event2", -1, 0, {0}, NULL, NULL, NULL},
    {TM_PANEL_NONE,                 NULL,  0, 0, {0}, NULL, NULL, NULL}
};

void tm_recv_event(const char *from,unsigned int len,unsigned char *msg)
{
    // hdr, number of sentence, sentence, panel, st_x, st_y, w, h, ap, st_x, st_y, w, h
    q_dbg("recv len %d, from %s", len, from);

    if((len != TM_IPC_CONG_LEN) || (msg[0] != TM_IPC_HDR_SET))
        return;

    unsigned char max = msg[1];
    unsigned char num = msg[2];
    unsigned char panel_id = msg[3];
    unsigned char ap_id = msg[8];

    if((num>max) || (panel_id > TM_PANEL_NUM) || (ap_id > TM_AP_NUM))
        return;

    tm_display_t* dis = (tm_display_t*)q_calloc(sizeof(tm_display_t));

    dis->ap = &tm.ap[ap_id];

    tm_set_fb_param(&dis->to, msg[4], msg[5], msg[6], msg[7]);
    tm_set_fb_param(&dis->from, msg[9], msg[10], msg[11], msg[12]);

    tm_fill_up_fb_conf(&dis->from, tm.ap[ap_id].native_size);
    tm_fill_up_fb_conf(&dis->to, tm.panel[panel_id].native_size);

    if(num == 1)
    {
        if(tm.dis_conf)
            q_free(tm.dis_conf);
        tm.dis_conf = dis;
    }
    else
    {
        tm_display_t* head = tm.dis_conf;

        while(head->next != NULL)
        {
            head = head->next;
        }

        head->next = dis;
        dis->next = NULL;
    }

    if(num == max)
    {
        _tm_remove_display_setting(panel_id);
        tm.panel[panel_id].head_display.next = tm.dis_conf;
        tm.dis_conf = NULL;
    }
}

void tm_set_fb_param(tm_fb_param_t* fb, int start_x, int start_y, int per_width, int per_high)
{
    q_assert(fb);

    fb->st_x = start_x;
    fb->st_y = start_y;
    fb->w = per_width;
    fb->h = per_high;
}

void _tm_remove_display_setting(tm_panel_t id)
{
    tm_display_t* dis = tm.panel[id].head_display.next;

    while(dis!=NULL)
    {
        q_mutex_lock(tm.panel[id].mutex);

        tm.panel[id].head_display.next = dis->next;
        q_free(dis);
        dis = tm.panel[id].head_display.next;

        q_mutex_unlock(tm.panel[id].mutex);
    }
}

void tm_remove_display_setting()
{
    int id;

    for(id=0; id<TM_PANEL_NUM; id++)
    {
        _tm_remove_display_setting((tm_panel_t)id);
    }
}

void tm_fill_up_fb_conf(tm_fb_param_t* fb, tm_native_size_param_t* native)
{
    q_assert(fb);
    q_assert(native);

    fb->abs_st_x = (fb->st_x * native->max_x) / 100;
    fb->abs_st_y = (fb->st_y * native->max_y) / 100;
    fb->abs_end_x = fb->abs_st_x + ((native->max_x * fb->w) / 100);
    fb->abs_end_y = fb->abs_st_y + ((native->max_y * fb->h) / 100);
}

void tm_set_default_display()
{
    int zip[TM_PANEL_NUM][2] = {{TM_PANEL_FRONT, TM_AP_QSI},
                                {TM_PANEL_LEFT, TM_AP_QSI_L},
                                {TM_PANEL_RIGHT, TM_AP_QSI_R}};
    int i, panel_id, ap_id;
    tm_display_t* dis;

    tm_remove_display_setting();

    for(i=0; i<TM_PANEL_NUM; i++)
    {
        panel_id = zip[i][0];
        ap_id    = zip[i][1];

        dis = q_calloc(sizeof(tm_display_t));

        if(dis == NULL)
            continue;

        dis->ap = &tm.ap[ap_id];
        dis->from.st_x = 0;
        dis->from.st_y = 0;
        dis->from.w = 100;
        dis->from.h = 100;
        dis->to.st_x = 0;
        dis->to.st_y = 0;
        dis->to.w = 100;
        dis->to.h = 100;

        tm_fill_up_fb_conf(&dis->from, tm.ap[ap_id].native_size);
        tm_fill_up_fb_conf(&dis->to, tm.panel[panel_id].native_size);

        tm.panel[panel_id].head_display.next = dis;
    }
}

tm_errno_t tm_init()
{
    int idx;
    tm_errno_t err_no;

    tm.ap       = g_ap;
    tm.panel    = g_panel;
    tm.mutex    = q_mutex_new(q_true, q_true);
    tm.dis_conf = NULL;
    tm.flag     = TM_IPC_STATUS_NONE;

    if((err_no = tm_mapping_create_handler()) != TM_ERRNO_SUCCESS)
    {
        q_dbg("tm_create : %s", tm_err_str(err_no));
        return err_no;
    }

    for(idx=0; tm.ap[idx].name != TM_AP_NONE; idx++)
    {
        tm.ap[idx].mutex = q_mutex_new(q_true, q_true);
    }

    for(idx=0; tm.panel[idx].name != TM_PANEL_NONE; idx++)
    {
        tm.panel[idx].mutex = q_mutex_new(q_true, q_true);
    }

    if((err_no = tm_input_init(tm.panel, tm.ap)) != TM_ERRNO_SUCCESS)
    {
        q_dbg("tm_create : %s", tm_err_str(err_no));
        return err_no;
    }

    tm_bind_param();
    tm_set_default_display();

    return TM_ERRNO_SUCCESS;
}

void tm_deinit()
{
    int idx;

    tm_remove_display_setting();

    for(idx=0; tm.ap[idx].name != TM_AP_NONE; idx++)
        if(tm.ap[idx].mutex) q_mutex_free(tm.ap[idx].mutex);

    for(idx=0; tm.panel[idx].name != TM_PANEL_NONE; idx++)
        if(tm.panel[idx].mutex) q_mutex_free(tm.panel[idx].mutex);

    tm_mapping_destroy_handler();
    tm_input_deinit();
}

void tm_set_status(tm_status_t status)
{
    q_mutex_lock(tm.mutex);
    *tm.status = status;
    q_mutex_unlock(tm.mutex);
}

void tm_bind_status(tm_status_t* status)
{
    q_assert(status);
    tm.status = status;
}

void tm_bind_param()
{
    // panel parameter pointer
    tm.panel[TM_PANEL_FRONT].cal_param    = tm_mapping_get_calibrate_param(TM_PANEL_FRONT);
    tm.panel[TM_PANEL_FRONT].native_size  = tm_mapping_get_native_size_param(TM_FB_800_480_0_0_0);
    tm.panel[TM_PANEL_LEFT].cal_param     = tm_mapping_get_calibrate_param(TM_PANEL_LEFT);
    tm.panel[TM_PANEL_LEFT].native_size   = tm_mapping_get_native_size_param(TM_FB_1000_600_0_0_0);
    tm.panel[TM_PANEL_RIGHT].cal_param    = tm_mapping_get_calibrate_param(TM_PANEL_RIGHT);
    tm.panel[TM_PANEL_RIGHT].native_size  = tm_mapping_get_native_size_param(TM_FB_1000_600_0_0_0);
    // frame buffer parameter pointer
    tm.ap[TM_AP_QSI].native_size     = tm_mapping_get_native_size_param(TM_FB_800_480_0_0_0);
    tm.ap[TM_AP_QSI_L].native_size   = tm_mapping_get_native_size_param(TM_FB_1000_600_0_0_0);
    tm.ap[TM_AP_QSI_R].native_size   = tm_mapping_get_native_size_param(TM_FB_1000_600_0_0_0);
    tm.ap[TM_AP_NAVI].native_size    = tm_mapping_get_native_size_param(TM_FB_800_480_0_0_0);
    tm.ap[TM_AP_MONITOR].native_size = tm_mapping_get_native_size_param(TM_FB_800_480_0_0_0);
}

tm_ap_info_t* tm_match_ap(int x, int y, tm_panel_info_t* panel)
{
    tm_display_t* dis=tm_match_display(x, y, panel);
    return (dis) ? dis->ap : NULL;
}

tm_display_t* tm_match_display(int x, int y, tm_panel_info_t* panel)
{
    tm_display_t* dis = NULL;

    for(dis = panel->head_display.next; dis != NULL; dis = dis->next)
    {
        if(x < dis->to.abs_st_x || x > dis->to.abs_end_x)
            continue;
        if(y < dis->to.abs_st_y || y > dis->to.abs_end_y)
            continue;
        break;
    }
    return dis;
}

tm_errno_t tm_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    // tm_transfer(x,y,event, panel)
    if (!x || !y)
        return TM_ERRNO_PARAM;

    panel = &tm.panel[1];

    return tm_mapping_transfer(x, y, panel);
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
