/*
 *  tm.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
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

tm_errno_t    tm_set_fb_param(tm_fb_param_t* fb, int start_x, int start_y, int per_width, int per_high);
void          tm_remove_display(tm_panel_info_t* panel);
void          tm_remove_all_display(void);
tm_ap_info_t* tm_get_default_ap(int panel_id);
void          tm_set_default_display(void);
tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel);


tm_errno_t tm_init()
{
    tm_errno_t err_no;

    q_init_head(&tm.ap_head);
    q_init_head(&tm.pnl_head);

    tm.mutex    = q_mutex_new(q_true, q_true);
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

    fb->abs_st_x = (fb->st_x * native->max_x) / 100;
    fb->abs_st_y = (fb->st_y * native->max_y) / 100;
    fb->abs_end_x = fb->abs_st_x + ((native->max_x * fb->w) / 100);
    fb->abs_end_y = fb->abs_st_y + ((native->max_y * fb->h) / 100);
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

    list_for_each_entry(&panel->display_head, dis, node)
    {

        if(tm_point_is_in_range(&dis->to, x, y))
            break;
        else
            q_dbg(Q_INFO,"(%d,%d) no match (%d - %d,%d - %d)", 
                    x,y,dis->to.abs_st_x, dis->to.abs_end_x, dis->to.abs_st_y, dis->to.abs_end_y);

    }
    return dis;
}

tm_ap_info_t* tm_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    if (!x || !y)
        return NULL;
    return tm_mapping_transfer(x, y, panel);
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

    if(tm_set_fb_param(&dis->to, msg[1], msg[2], msg[3], msg[4]) != TM_ERRNO_SUCCESS)
    {
        q_free(dis);
        return;
    }

    if(tm_set_fb_param(&dis->from, msg[6], msg[7], msg[8], msg[9]) != TM_ERRNO_SUCCESS)
    {
        q_free(dis);
        return;
    }
    
    tm_fillup_fb_param(&dis->from, dis->ap->native_size);
    tm_fillup_fb_param(&dis->to, panel->native_size);

    q_list_add(&panel->display_head, &dis->node);
    panel->link_num++;
    tm_mapping_print_panel_info(&tm.pnl_head);
}


tm_errno_t tm_set_fb_param(tm_fb_param_t* fb, int start_x, int start_y, int per_width, int per_high)
{
    q_assert(fb);

    if(start_x + per_width > 100 || start_y + per_high > 100)
        return TM_ERRNO_DEV_PARAM;

    fb->st_x = start_x;
    fb->st_y = start_y;
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
        if(panel->display_head.next != NULL)
            continue;

        dis = q_calloc(sizeof(tm_display_t));

        if(dis == NULL)
            continue;

        dis->ap = tm_get_default_ap(panel->id);

        dis->from.st_x = 0;
        dis->from.st_y = 0;
        dis->from.w = 100;
        dis->from.h = 100;
        dis->to.st_x = 0;
        dis->to.st_y = 0;
        dis->to.w = 100;
        dis->to.h = 100;

        tm_fillup_fb_param(&dis->from, dis->ap->native_size);
        tm_fillup_fb_param(&dis->to, panel->native_size);

        q_list_add(&panel->display_head, &dis->node);

    }
}

