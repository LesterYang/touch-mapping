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
#include <sys/types.h>
#include <sys/errno.h>
#include <linux/input.h>

#include "tmInput.h"
#include "qUtils.h"
#include "tm.h"
#include "tmMapping.h"

typedef struct _tm_input_dev {
    tm_panel_info_t*  panel;
    int               maxfd;
    fd_set            evfds;
    tm_input_status_t status;

	q_thread*         thread;
	list_head_t	      node;
    q_queue*          queue;
}tm_input_dev_t;

typedef struct _tm_input_handler {
    q_bool             open;
    list_head_t* 	   tm_ap_head;

	uint8_t			   dev_num;
	list_head_t        dev_head;
}tm_input_handler_t;

static tm_input_handler_t tm_input;

void tm_send_event(tm_input_dev_t* dev, tm_input_event_t* evt);

void tm_input_clean_stdin();
int  tm_input_init_events(list_head_t* pnl_head);
void tm_input_close_events();
void tm_input_remove_dev();

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

tm_errno_t tm_input_init(list_head_t* ap_head, list_head_t* pnl_head)
{
    int idx;

    tm_errno_t err_no;
    q_assert(ap_head);
    q_assert(pnl_head);

    q_init_head(&tm_input.dev_head);

    tm_input.open       = q_true;
    tm_input.tm_ap_head	= ap_head;
    
    if((err_no = tm_input_init_events(pnl_head)) != TM_ERRNO_SUCCESS)
        return err_no;

    return TM_ERRNO_SUCCESS;
}

void tm_input_deinit()
{
    tm_input.open = q_false;
    tm_input_close_events();
    tm_input_remove_dev();
}

tm_errno_t  tm_input_init_events(list_head_t* pnl_head)
{
    int idx;
    tm_ap_info_t* ap = NULL;
    tm_panel_info_t* panel = NULL;
    tm_input_dev_t* dev;

    tm_input_close_events();

    list_for_each_entry(tm_input.tm_ap_head, ap, node)
    {
        if((ap->fd = open (ap->evt_path, O_RDONLY )) < 0)
            q_dbg("Opened %s error",ap->evt_path);
        else
        	q_dbg("Opened %s done",ap->evt_path);
    }

    tm_input_clean_stdin();

    list_for_each_entry(pnl_head, panel, node)
    {
    	dev = (tm_input_dev_t*)q_calloc(sizeof(tm_input_dev_t));

    	if(dev == NULL)
    		continue;

    	dev->panel = panel;
    	dev->queue = panel->queue;
    	q_list_add(&tm_input.dev_head, &dev->node);
    	tm_input.dev_num++;

        if((panel->fd = open (panel->evt_path, O_RDONLY )) < 0)
        {
            q_dbg("Opened %s error",panel->evt_path);
        }
        else
        {
        	q_dbg("Opened %s done",panel->evt_path);
        	dev->status = TM_INPUT_STATUS_NONE;
        	dev->thread = q_thread_new(tm_input_thread_func, dev);
        }
    }

    return TM_ERRNO_SUCCESS;
}

void tm_input_close_events()
{
    int idx;
    tm_ap_info_t* ap = NULL;
    tm_input_dev_t* dev = NULL;

    list_for_each_entry(tm_input.tm_ap_head, ap, node)
    {
    	if(ap->fd > 0)
    	{
    		q_close(ap->fd);
    		ap->fd = -1;
    	}
    }

    list_for_each_entry(&tm_input.dev_head, dev, node)
    {
    	if(dev->panel != NULL && dev->panel->fd > 0)
    	{
    		q_close(dev->panel->fd);
    		dev->panel->fd = -1;
    	}
    }
}

void tm_input_remove_dev()
{
    tm_input_dev_t* dev;
    tm_input.dev_num = 0;

    tm_input.open = q_false;

    while((dev = list_first_entry(&tm_input.dev_head, tm_input_dev_t, node)) != NULL)
    {
    	if(dev->thread)
    	{
    		q_thread_free(dev->thread);
    		dev->thread = NULL;
    	}

    	q_list_del(&dev->node);
    	q_free(dev);
    }
}

void tm_send_event(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    q_dbg("%s, code:%3d value:%3d",dev->panel->evt_path, evt->code, evt->value);

}

void tm_input_parse_event(tm_input_dev_t* dev)
{
    static tm_input_event_t last_evt[64];
    static int x, y, idx_x, idx_y, evt_num;
    static tm_ap_info_t* ap;

    char buf[sizeof(tm_input_event_t)] = {0};
    tm_input_event_t* evt = (tm_input_event_t *)buf;

    if ((dev->panel->fd < 0) || !(FD_ISSET(dev->panel->fd, &dev->evfds)))
        return;

    if(tm_input.open == q_false)
        return;

    if(q_loop_read(dev->panel->fd, buf, sizeof(tm_input_event_t)) != (ssize_t)sizeof(tm_input_event_t))
        return;

    switch (evt->type)
    {
        case EV_SYN:
            dev->status = TM_INPUT_STATUS_END;
            break;
        case EV_KEY:
            if(evt->code == BTN_TOUCH && evt->value)
                dev->status = TM_INPUT_STATUS_START;

            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_X:
                    idx_x = evt_num;
                    x = evt->value;
                    set_abs_status(dev->status);
                    break;
                case ABS_Y:
                    idx_y = evt_num;
                    y = evt->value;
                    set_abs_status(dev->status);
                    break;
                case ABS_MT_POSITION_X:
                    x = evt->value;
                    set_abs_status(dev->status);
                    break;
                case ABS_MT_POSITION_Y:
                    y = evt->value;
                    set_abs_status(dev->status);
                    break;
                default:
                   dev->status = TM_INPUT_STATUS_PASS;
                    break;
            }
            break;
        default:
            dev->status = TM_INPUT_STATUS_PASS;
            break;
    }

    switch(dev->status)
    {
        case TM_INPUT_STATUS_START:
            evt_num = 0;
            memcpy(&last_evt[evt_num++], (const char*)buf, sizeof(tm_input_event_t));
            break;
        case TM_INPUT_STATUS_PASS:
            memcpy(&last_evt[evt_num++], evt, sizeof(tm_input_event_t));
            break;
        case TM_INPUT_STATUS_COLLECT:
        case TM_INPUT_STATUS_MT_COLLECT:
            memcpy(&last_evt[evt_num++], evt, sizeof(tm_input_event_t));
            break;
        case TM_INPUT_STATUS_TRANS:
            memcpy(&last_evt[evt_num++], evt, sizeof(tm_input_event_t));
            if(idx_x > 0 && idx_x < evt_num && idx_y < evt_num && idx_y > 0)
            {
                //q_dbg("in  : %d, %d\n",last_evt[idx_x].value, last_evt[idx_y].value);
                ap = tm_transfer(&last_evt[idx_x].value, &last_evt[idx_y].value, dev->panel);
                //q_dbg("out : %d, %d\n",last_evt[idx_x].value, last_evt[idx_y].value);
            }
            dev->status = TM_INPUT_STATUS_PASS;
            break;
        case TM_INPUT_STATUS_MT_END:
            memcpy(&last_evt[evt_num++], evt, sizeof(tm_input_event_t));
            break;
        case TM_INPUT_STATUS_END:
            memcpy(&last_evt[evt_num++], evt, sizeof(tm_input_event_t));
            if(ap && ap->fd > 0)
            {
                int i;
                for(i=0; i<evt_num; i++)
                {
                    //q_dbg("type %4d code %4d value %4d",last_evt[i].type, last_evt[i].code, last_evt[i].value);
                    if(write(ap->fd, &last_evt[i], sizeof(tm_input_event_t)) == -1)
                        q_dbg("write %s error",ap->evt_path);
                }
            }
            ap = NULL;
            evt_num = 0;
            break;
        default:
            break;
    }
}

/*
#define Q_CMD_LEN           (2)
#define QUEUE_HDR           (0xff)
#define QUEUE_CMD_PASS      (0x55)
#define QUEUE_CMD_COLLECT   (0xa0)
#define QUEUE_CMD_TRANS     (0xb0)

#define QUEUE_DATA_SIZE (Q_CMD_LEN + sizeof(tm_input_event_t))
#define unlock_and_continue(mutex)      \
{                                       \
    q_mutex_unlock(mutex);              \
    continue;                           \
}
uint8_t g_cmd_pass[Q_CMD_LEN]={QUEUE_HDR, QUEUE_CMD_PASS};
uint8_t g_cmd_collect[Q_CMD_LEN]={QUEUE_HDR, QUEUE_CMD_COLLECT};
uint8_t g_cmd_trans[Q_CMD_LEN]={QUEUE_HDR, QUEUE_CMD_TRANS};

void tm_input_save_event(tm_input_dev_t* dev, tm_input_event_t* ev)
{
    q_dbg("%d -> type : %2x, code : %2x, value : %2x",dev->panel->evt_path, ev->type, ev->code, ev->value);

    if(tm_input.open == q_false)
        return;

    q_mutex_lock(dev->panel->mutex);

    if(dev->status == TM_INPUT_STATUS_COLLECT)
        q_set_queue(dev->queue, g_cmd_collect, sizeof(g_cmd_collect), q_true);
    else if((dev->status == TM_INPUT_STATUS_TRANS))
        q_set_queue(dev->queue, g_cmd_trans, sizeof(g_cmd_trans), q_true);
    else
        q_set_queue(dev->queue, g_cmd_pass, sizeof(g_cmd_pass), q_true);
    
    q_set_queue(dev->queue, ev, sizeof(tm_input_event_t), q_true);
 
    q_mutex_unlock(dev->panel->mutex);

    return;
}

// at tm.c
void tm_parse_event()
{
    char* buf;
    char* buf2;
    char hdr;

    tm_input_dev_t* dev;
    tm_input_event_t ev;
    static tm_input_event_t last_ev;

    while(q_size_queue(tm->queue) >= QUEUE_DATA_SIZE)
    {
        q_mutex_lock(tm->mutex);

        if( q_pop_queue(tm->queue, &hdr) || hdr != (char)QUEUE_HDR )
            unlock_and_continue(tm->mutex);

        if( q_pop_queue(tm->queue, &hdr) || (hdr != (char)QUEUE_CMD_PASS && hdr != (char)QUEUE_CMD_COLLECT && hdr != (char)QUEUE_CMD_TRANS) )
            unlock_and_continue(tm->mutex);


        buf = (char*)&ev;
        if(q_get_queue(tm->queue, buf, sizeof(tm_input_event_t)) != sizeof(tm_input_event_t))
            unlock_and_continue(tm->mutex);


        q_mutex_unlock(tm->mutex);

        if(hdr == QUEUE_CMD_COLLECT)
        {
            memcpy(&last_ev, &ev, sizeof(tm_input_event_t));
        }
        else if (hdr == QUEUE_CMD_TRANS)
        {
            int *x, *y;

            if(last_ev.code == ABS_X && ev.code == ABS_Y )
            {
                x = &last_ev.value;
                y = &ev.value;
            }
            else if (last_ev.code == ABS_Y && ev.code == ABS_X)
            {
                y = &last_ev.value;
                x = &ev.value;
            }
            else
                continue;
            
            tm_transfer(&x, &y, dev);
            tm_send_event(dev, &last_ev);
            tm_send_event(dev, &ev);
        }
        else
        {
            tm_send_event(dev, &ev);
        }
    }

}

*/


void tm_input_thread_func(void *data)
{
    struct timeval tv;
    int ret;
    tm_input_dev_t* dev = (tm_input_dev_t*)data;

    q_dbg("thread run : name : %d %s",dev->panel->name, dev->panel->evt_path);

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

