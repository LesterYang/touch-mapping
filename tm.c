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

    tm_display_t*       dis_conf;
    tm_ipc_status_t     flag;
	
	list_head_t       	ap_head;
	list_head_t       	pnl_head;
	q_mutex*            mutex;
}tm_info_t;

static tm_info_t tm;

tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel);

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

void _tm_remove_display_conf(tm_panel_info_t* panel)
{
	tm_display_t* dis;
	while((dis = list_first_entry(&panel->display_head, tm_display_t, node)) != NULL)
	{
    	q_list_del(&dis->node);
    	q_free(dis);
	}
}

void tm_remove_display_conf()
{
	tm_panel_info_t* panel;

    list_for_each_entry(&tm.pnl_head, panel, node)
    {
    	_tm_remove_display_conf(panel);
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
	tm_display_t* dis;
	tm_panel_info_t* panel;

	tm_remove_display_conf();

    list_for_each_entry(&tm.pnl_head, panel, node)
    {
        dis = q_calloc(sizeof(tm_display_t));

        if(dis == NULL)
            continue;

        dis->ap = tm_mapping_get_default_ap(panel->id);

        dis->from.st_x = 0;
        dis->from.st_y = 0;
        dis->from.w = 100;
        dis->from.h = 100;
        dis->to.st_x = 0;
        dis->to.st_y = 0;
        dis->to.w = 100;
        dis->to.h = 100;

        tm_fill_up_fb_conf(&dis->from, dis->ap->native_size);
        tm_fill_up_fb_conf(&dis->to, panel->native_size);

        q_list_add(&panel->display_head, &dis->node);

    }
}

tm_ap_info_t* tm_mapping_get_default_ap(int panel_id)
{
	tm_ap_info_t* ap = tm_mapping_get_ap_info(panel_id);

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

tm_errno_t tm_init()
{
    tm_errno_t err_no;

    q_init_head(&tm.ap_head);
    q_init_head(&tm.pnl_head);

    tm.mutex    = q_mutex_new(q_true, q_true);
    tm.dis_conf = NULL;
    tm.flag     = TM_IPC_STATUS_NONE;

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

    tm_mapping_print_conf(&tm.ap_head, &tm.pnl_head);

    return TM_ERRNO_SUCCESS;
}

void tm_deinit()
{
    tm_ap_info_t *ap;
    tm_panel_info_t *panel;

    tm_input_deinit();
    tm_remove_display_conf();
    tm_mapping_destroy_handler();

    while((ap = list_first_entry(&tm.ap_head, tm_ap_info_t, node)) != NULL)
    {
    	q_list_del(&ap->node);
    	q_free((char*)ap->evt_path);
    	if(ap->mutex) q_mutex_free(ap->mutex);
    	q_free(ap);
    }

    while((panel = list_first_entry(&tm.pnl_head, tm_panel_info_t, node)) != NULL)
    {
    	q_list_del(&panel->node);
    	q_free((char*)panel->evt_path);
    	if(panel->mutex) q_mutex_free(panel->mutex);
    	q_free(panel);
    }
	
	if(tm.mutex)
		q_mutex_free(tm.mutex);
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

void tm_set_map(unsigned int len, unsigned char *msg, q_bool append)
{
    unsigned char pnl_id, ap_id;
    tm_panel_info_t* panel;
    tm_ap_info_t* ap;
    tm_display_t* dis;

    if(len != IPC_MAP_CONF_LEN)
        return;

    // panel,start_x,start_y,per_width,per_high,ap,start_x,start_y,per_width,per_high

    pnl_id = msg[0];
    ap_id = msg[5];

    panel = tm_mapping_get_panel_info(pnl_id);
    ap = tm_mapping_get_ap_info(ap_id);

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

    tm_fill_up_fb_conf(&dis->from, dis->ap->native_size);
    tm_fill_up_fb_conf(&dis->to, panel->native_size);

    if(append == q_false)
        _tm_remove_display_conf(panel);

    q_list_add_tail(&panel->display_head, &dis->node);
}

tm_ap_info_t* tm_mapping_get_ap_info(int id)
{
	tm_ap_info_t* ap = NULL;

    list_for_each_entry(&tm.ap_head, ap, node)
    {
    	 if(ap->id == id) break;
    }
    return ap;
}

tm_panel_info_t* tm_mapping_get_panel_info(int id)
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
    }
    return dis;
}

tm_ap_info_t* tm_transfer(int *x, int *y, tm_panel_info_t* panel)
{
 
    if (!x || !y)
        return NULL;

    // for test
    panel = tm_mapping_get_panel_info(0);
    
    return tm_mapping_transfer(x, y, panel);
}

//========================================================================
void tm_dbg_print_conf()
{
    tm_mapping_print_conf(&tm.ap_head, &tm.pnl_head);
}

