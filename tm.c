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


typedef struct _tm_info
{
    tm_status_t*        status;
    tm_ap_info_t*       ap;
    tm_panel_info_t*    panel;

    tm_display_t*       dis_conf;
    tm_ipc_status_t     flag;
	
	list_head_t       	ap_head;
	list_head_t       	panel_head;
	q_mutex*            mutex;
}tm_info_t;

static tm_info_t tm;

tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel);

// need to remove. set by configure file
tm_ap_info_t g_ap[] = {
    {TM_AP_QSI,     "/dev/input/event20", -1, NULL, {0}, NULL},
    {TM_AP_QSI_L,   "/dev/input/event21", -1, NULL, {0}, NULL},
    {TM_AP_QSI_R,   "/dev/input/event22", -1, NULL, {0}, NULL},
    {TM_AP_NAVI,    "/dev/input/event23", -1, NULL, {0}, NULL},
    {TM_AP_MONITOR, "/dev/input/event24", -1, NULL, {0}, NULL},
    {TM_AP_NONE,                    NULL,  0, NULL}
};

tm_panel_info_t g_panel[] = {
    {TM_PANEL_FRONT, "/dev/input/event0", -1, 0, {0}, NULL, NULL, {0}, {0}, NULL, NULL},
    {TM_PANEL_LEFT,  "/dev/input/event1", -1, 0, {0}, NULL, NULL, {0}, {0}, NULL, NULL},
    {TM_PANEL_RIGHT, "/dev/input/event2", -1, 0, {0}, NULL, NULL, {0}, {0}, NULL, NULL},
    {TM_PANEL_NONE,                 NULL,  0, 0, {0}, NULL, NULL, {0}, {0}, NULL, NULL}
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

    // need modify : set by configure file
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
        tm.panel[idx].queue = q_create_queue(MAX_QUEUE);
        q_destroy_queue(tm.panel[idx].queue);
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
    // need to remove tm_ap_t, tm_fb_t, tm_panel_t
    // all depend on id by configure file

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
        if(tm_point_is_in_range(&dis->to, x, y))
            break;
    }
    return dis;
}

tm_ap_info_t* tm_transfer(int *x, int *y, tm_panel_info_t* panel)
{
 
    if (!x || !y)
        return NULL;

    panel = &tm.panel[1];
    
    return tm_mapping_transfer(x, y, panel);
}

