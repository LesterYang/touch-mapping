/*
 *  tm.c
 *  Copyright © 2014  
 *  All rights reserved.
 *  
 *       Author : Lester Yang <sab7412@yahoo.com.tw>
 *  Description : tm-daemon manger
 */
#include <string.h>
#include "tm.h"
#include "tmIpc.h"

typedef struct _tm_info
{
    tm_display_t*       dis_conf;

    list_head_t       	ap_head;
    list_head_t       	pnl_head;
    q_mutex*            mutex;
}tm_info_t;

static tm_info_t tm;

tm_errno_t    tm_set_fb_param(tm_fb_param_t* fb, int per_st_x, int per_st_y, int per_width, int per_high);
void          tm_remove_display(tm_panel_info_t* panel);
void          tm_remove_all_display(void);
tm_ap_info_t* tm_get_default_ap(int panel_id);
void          tm_set_default_display(void);
tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel);
void          tm_mapping_duplicate_transfer(int *x, int *y, tm_panel_info_t* panel);

tm_errno_t tm_init()
{
    tm_errno_t err_no;

    q_init_head(&tm.ap_head);
    q_init_head(&tm.pnl_head);

    tm.mutex    = q_mutex_new(true, true);
    tm.dis_conf = NULL;

    // do check
    if((err_no = tm_mapping_create_handler(&tm.ap_head, &tm.pnl_head)) != TM_ERRNO_SUCCESS)
    {
        q_dbg(Q_ERR,"tm_create : %s", tm_err_str(err_no));
        if(tm.mutex)
            q_mutex_free(tm.mutex);
        return err_no;
    }

    if((err_no = tm_input_init(&tm.ap_head, &tm.pnl_head)) != TM_ERRNO_SUCCESS)
    {
        q_dbg(Q_ERR,"tm_create : %s", tm_err_str(err_no));
        return err_no;
    }

    tm_set_default_display();

    tm_mapping_print_config(&tm.ap_head, &tm.pnl_head);

    return TM_ERRNO_SUCCESS;
}

void tm_deinit()
{
    tm_input_deinit();
    tm_remove_all_display();
    tm_mapping_destroy_handler(&tm.ap_head, &tm.pnl_head);
	
    if(tm.mutex)
        q_mutex_free(tm.mutex);
}

void tm_fillup_fb_param(tm_fb_param_t* fb, tm_native_size_param_t* native)
{
    q_assert(fb);
    q_assert(native);

    fb->abs_begin_x = (fb->begin_x * native->max_x) / 100;
    fb->abs_begin_y = (fb->begin_y * native->max_y) / 100;
    fb->abs_end_x = fb->abs_begin_x + ((native->max_x * fb->w) / 100);
    fb->abs_end_y = fb->abs_begin_y + ((native->max_y * fb->h) / 100);
}

tm_ap_info_t* tm_get_ap_info(int id)
{
    tm_ap_info_t* ap = NULL;

    list_for_each_entry(&tm.ap_head, ap, node)
    {
    	 if(ap->id == id) break;
    }
    return ap;
}

tm_panel_info_t* tm_get_panel_info(int id)
{
    tm_panel_info_t* panel = NULL;

    list_for_each_entry(&tm.pnl_head, panel, node)
    {
        if(panel->id == id) break;
    }
    return panel;
}

tm_ap_info_t* tm_match_ap(int x, int y, tm_panel_info_t* panel)
{
    tm_display_t* dis=tm_match_display(x, y, panel);
    return (dis) ? dis->ap : NULL;
}

tm_display_t* tm_match_display(int x, int y, tm_panel_info_t* panel)
{
    tm_display_t* dis = NULL;

    q_dbg(Q_DBG_CHK_MATCH,"panel id %d search %d %d ->",panel->id, x, y);

    list_for_each_entry(&panel->display_head, dis, node)
    {  
        q_dbg(Q_DBG_CHK_MATCH,"                  x:%4d~%4d y:%4d~%4d"
                                ,dis->to_pnl.abs_begin_x 
                                ,dis->to_pnl.abs_end_x
                                ,dis->to_pnl.abs_begin_y
                                ,dis->to_pnl.abs_end_y);
    
        if(tm_point_is_in_range(&dis->to_pnl, x, y))
        {
             q_dbg(Q_DBG_CHK_MATCH,"matched ap id %d", dis->ap->id);
            break;
        }
    }

    if(dis == NULL)
    {
        q_dbg(Q_INFO,"(%d,%d) no match ap", x, y);
    }

    return dis;
}

tm_ap_info_t* tm_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    if (!x || !y)
        return NULL;
    return tm_mapping_transfer(x, y, panel);
}

void tm_duplicate_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    if (!x || !y)
        return;
   tm_mapping_duplicate_transfer(x, y, panel);
   return;
}


void tm_return_version(unsigned int len, char* from)
{
    if(len != IPC_GET_VER_LEN)
        return;
    
    int size = sizeof(TM_VERSION) + 1;
    unsigned char *ver = (unsigned char*)q_malloc(size);

    ver[0]=IPC_CMD_GET_VER;
    memcpy(&ver[1], TM_VERSION, size-1);
    tm_ipc_send(from, ver, (int)sizeof(TM_VERSION));
    free(ver);
}

void tm_clear_map(unsigned int len, unsigned char *msg)
{
    unsigned char pnl_id;
    tm_panel_info_t* panel;

    if(len != IPC_CLR_MAP_LEN)
        return;
    
    pnl_id = msg[0];

    if((panel = tm_get_panel_info(pnl_id)) == NULL)
        return;

    tm_remove_display(panel);
    q_dbg(Q_INFO,"panel id %d clear all display ap", pnl_id);
    panel->link_num=0;
}

void tm_set_map(unsigned int len, unsigned char *msg)
{
    unsigned char pnl_id, ap_id;
    tm_panel_info_t* panel;
    tm_ap_info_t* ap;
    tm_display_t* dis;

    if(len != IPC_SET_MAP_LEN)
        return;

    // panel,start_x,start_y,per_width,per_high,ap,start_x,start_y,per_width,per_high

    pnl_id = msg[0];
    ap_id = msg[5];

    panel = tm_get_panel_info(pnl_id);
    ap = tm_get_ap_info(ap_id);

    if(panel == NULL || ap == NULL)
        return;

    dis = (tm_display_t*)q_calloc(sizeof(tm_display_t));

    dis->ap = ap;

    if(tm_set_fb_param(&dis->to_pnl, msg[1], msg[2], msg[3], msg[4]) != TM_ERRNO_SUCCESS)
    {
        q_free(dis);
        return;
    }

    if(tm_set_fb_param(&dis->from_ap, msg[6], msg[7], msg[8], msg[9]) != TM_ERRNO_SUCCESS)
    {
        q_free(dis);
        return;
    }
    
    tm_fillup_fb_param(&dis->from_ap, dis->ap->native_size);
    tm_fillup_fb_param(&dis->to_pnl, panel->native_size);

    q_list_add(&panel->display_head, &dis->node);
    panel->link_num++;
    tm_mapping_print_panel_info(&tm.pnl_head);
}

void tm_update_ap_native_size(unsigned int len, unsigned char *msg)
{
    tm_ap_info_t* ap = NULL;
    tm_display_t* dis;
    tm_panel_info_t* panel;

    if(len != IPC_CMD_SET_APSIZE_LEN)
        return;

    ap = tm_get_ap_info(msg[0]);

    if(!ap || !ap->native_size)
        return;

    if(tm_mapping_native_size_is_const(ap->native_size))
        return;
    
    ap->native_size->max_x = 256 * msg[1] + msg[2];
    ap->native_size->max_y = 256 * msg[3] + msg[4];

    // update display setting
    list_for_each_entry(&tm.pnl_head, panel, node)
    {
        list_for_each_entry(&panel->display_head, dis, node)
        {
            if(dis->ap == ap)
                tm_fillup_fb_param(&dis->from_ap, ap->native_size);
        }
    }
      tm_mapping_print_panel_info(&tm.pnl_head);
}

void tm_switch_ap_threshold(unsigned int len, unsigned char *msg)
{
    tm_ap_info_t* ap = NULL;
    
    if(len != IPC_CMD_EVT_INTR_LEN)
        return;

    ap = tm_get_ap_info(msg[0]);

    if(ap)
        ap->threshold = (bool)msg[1];

    q_dbg(Q_DBG_THRESHOLD,"ap id %2d threshold : %d", ap->id, ap->threshold);
}


tm_errno_t tm_set_fb_param(tm_fb_param_t* fb, int per_st_x, int per_st_y, int per_width, int per_high)
{
    q_assert(fb);

    if(per_st_x + per_width > 100 || per_st_y + per_high > 100)
        return TM_ERRNO_DEV_PARAM;

    fb->begin_x = per_st_x;
    fb->begin_y = per_st_y;
    fb->w = per_width;
    fb->h = per_high;

    return TM_ERRNO_SUCCESS;
}

void tm_remove_display(tm_panel_info_t* panel)
{
    tm_display_t* dis;
    while((dis = list_first_entry(&panel->display_head, tm_display_t, node)) != NULL)
    {
        q_list_del(&dis->node);
        q_free(dis);
    }
}

void tm_remove_all_display()
{
    tm_panel_info_t* panel;

    list_for_each_entry(&tm.pnl_head, panel, node)
    {
    	tm_remove_display(panel);
    }
}

tm_ap_info_t* tm_get_default_ap(int panel_id)
{
    // default : get ap info by ap id the same as panel id
    tm_ap_info_t* ap = tm_get_ap_info(panel_id);

    if(ap == NULL)
    {
        list_for_each_entry(&tm.ap_head, ap, node)
        {
             if(ap != NULL)
                 break;
        }
    }
    return ap;
}

void tm_set_default_display()
{
    tm_display_t* dis;
    tm_panel_info_t* panel;


    list_for_each_entry(&tm.pnl_head, panel, node)
    {
        // check panel has display setting or not 
        if(panel->display_head.next != NULL)
            continue;

        dis = q_calloc(sizeof(tm_display_t));

        if(dis == NULL)
            continue;

        dis->ap = tm_get_default_ap(panel->id);

        dis->from_ap.begin_x= 0;
        dis->from_ap.begin_y = 0;
        dis->from_ap.w = 100;
        dis->from_ap.h = 100;
        dis->to_pnl.begin_x = 0;
        dis->to_pnl.begin_y = 0;
        dis->to_pnl.w = 100;
        dis->to_pnl.h = 100;

        tm_fillup_fb_param(&dis->from_ap, dis->ap->native_size);
        tm_fillup_fb_param(&dis->to_pnl, panel->native_size);

        q_list_add(&panel->display_head, &dis->node);

    }
}

