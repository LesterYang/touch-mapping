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
#include <mtdev.h>

#include "tmInput.h"
#include "qUtils.h"
#include "tm.h"
#include "tmMapping.h"

#define BUF_EVT_NUM (64)

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif

#define SLOT_NUM (5)

typedef struct _tm_input_coord{
        int x;
        int y;
        int p;
}tm_input_coord_t;


typedef struct _tm_input_queue{
        tm_input_event_t  evt[BUF_EVT_NUM];
        int idx_x;
        int idx_y;
        int evt_num;
        tm_ap_info_t* ap;
        int slot;
        tm_input_coord_t mt[SLOT_NUM];
}tm_input_queue_t;

typedef struct _tm_input_dev {
    tm_panel_info_t*  panel;
    int               maxfd;
    fd_set            evfds;
    tm_input_type_t   type;

    tm_input_status_t status;
    tm_ap_info_t**    act_ap;
    uint8_t           max_act_num;

	q_thread*         thread;
	list_head_t	      node;

    q_queue*          queue;
    tm_input_queue_t  input_queue;
}tm_input_dev_t;

typedef struct _tm_input_handler {
    q_bool             open;
    list_head_t* 	   tm_ap_head;

    uint8_t            ap_num;
	uint8_t			   dev_num;

	list_head_t        dev_head;
}tm_input_handler_t;

static tm_input_handler_t tm_input;

void tm_send_event(tm_input_dev_t* dev, q_bool all);

void tm_input_clean_stdin();
int  tm_input_init_events(list_head_t* pnl_head);
void tm_input_close_events();
void tm_input_remove_dev();

int  tm_input_add_fd (tm_panel_info_t* tm_input, fd_set * fdsp);

void tm_input_thread_func(void *data);

const char* tm_input_evt_str(int type,int code)
{
    if(type == EV_ABS)
    {
        switch(code)
        {
            case ABS_X:             return "type:ABS, code:X";
            case ABS_Y:             return "type:ABS, code:Y";
            case ABS_PRESSURE:      return "type:ABS, code:PRESSURE";
            case ABS_MT_SLOT:       return "type:ABS, code:SLOT";
            case ABS_MT_TOUCH_MAJOR:return "type:ABS, code:MAJOR";
            case ABS_MT_POSITION_X: return "type:ABS, code:MT_X";
            case ABS_MT_POSITION_Y: return "type:ABS, code:MT_Y";
            case ABS_MT_TRACKING_ID:return "type:ABS, code:TRK_ID";
            default:                return "type:ABS, code:Unknown";
        }
    }
    else if(type == EV_SYN)
    {
        switch(code)
        {
            case SYN_REPORT:        return "type:SYN, code:SYN";
            case SYN_MT_REPORT:     return "type:SYN, code:MT_SYN";
            default:                return "type:SYN, code:Unknown";
        }
    }
    else if(type == EV_KEY)
    {
        switch(code)
        {
            case BTN_TOUCH:         return "type:KEY, code:TOUCH";
            case BTN_TOOL_FINGER:   return "type:KEY, code:1_FINGER";
            case BTN_TOOL_DOUBLETAP:return "type:KEY, code:2_FINGERS";
            case BTN_TOOL_TRIPLETAP:return "type:KEY, code:3_FINGERS";
            case BTN_TOOL_QUADTAP:  return "type:KEY, code:4_FINGERS";
            default:                return "type:KEY, code:XXXX";
        }
    }

    return "type:XXXX, code:Unknown";
}


void tm_input_add_time(tm_input_timeval_t *time, int ms)
{
    time->tv_usec += ms;

    if((time->tv_usec / 1000000))
    {
        time->tv_sec++;
    }
    time->tv_usec %= 1000000;
}

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

void tm_input_set_type(tm_input_dev_t* dev)
{
    long absbits[NUM_LONGS(ABS_CNT)]={0};

    if (ioctl(dev->panel->fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0)
    {
        if(testBit(ABS_MT_SLOT, absbits))
            dev->type = TM_INPUT_TYPE_MT_B;
        else if(testBit(ABS_MT_POSITION_X, absbits))
            dev->type = TM_INPUT_TYPE_MT_A;
        else
            dev->type = TM_INPUT_TYPE_SINGLE;
    }
}

tm_errno_t  tm_input_init_events(list_head_t* pnl_head)
{
    tm_ap_info_t* ap = NULL;
    tm_panel_info_t* panel = NULL;
    tm_input_dev_t* dev;

    tm_input_close_events();

    list_for_each_entry(tm_input.tm_ap_head, ap, node)
    {
        tm_input.ap_num++;

        if((ap->fd = open (ap->evt_path, O_WRONLY )) < 0)
            q_dbg(Q_ERR,"Opened %s error",ap->evt_path);
        else
        	q_dbg(Q_DBG,"Opened %s done",ap->evt_path);
    }

    tm_input_clean_stdin();

    list_for_each_entry(pnl_head, panel, node)
    {
    	dev = (tm_input_dev_t*)q_calloc(sizeof(tm_input_dev_t));

    	if(dev == NULL)
    		continue;

    	dev->panel = panel;
    	dev->queue = panel->queue;
    	dev->max_act_num = tm_input.ap_num ;
        if(dev->max_act_num)
            dev->act_ap = (tm_ap_info_t**)q_calloc(dev->max_act_num * sizeof(tm_ap_info_t*));

        q_list_add(&tm_input.dev_head, &dev->node);
    	tm_input.dev_num++;

        if((panel->fd = open (panel->evt_path, O_RDONLY | O_NONBLOCK)) < 0)
        {
            q_dbg(Q_ERR,"Opened %s error",panel->evt_path);
        }
        else
        {
        	q_dbg(Q_DBG,"Opened %s done",panel->evt_path);
        	dev->status = TM_INPUT_STATUS_NONE;
        	dev->thread = q_thread_new(tm_input_thread_func, dev);
        }
        tm_input_set_type(dev);
    }

    return TM_ERRNO_SUCCESS;
}

void tm_input_close_events()
{
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

    	q_free(dev->act_ap);
    	q_list_del(&dev->node);
    	q_free(dev);
    }
}

void tm_add_active_ap(tm_input_dev_t* dev, tm_ap_info_t* ap)
{
    int idx;

    for(idx=0; idx<dev->max_act_num; idx++)
    {
        if(!dev->act_ap[idx])
        {
            dev->act_ap[idx] = ap;
            break;
        }
    }
}

void tm_del_active_ap(tm_input_dev_t* dev, tm_ap_info_t* ap)
{
    int idx;

    for(idx=0; idx<dev->max_act_num; idx++)
    {
        if(dev->act_ap[idx] == ap)
        {
            dev->act_ap[idx++] = NULL;
            break;
        }
    }

    while(idx < dev->max_act_num)
    {
        if(dev->act_ap[idx])
        {
            dev->act_ap[idx-1] = dev->act_ap[idx];
            dev->act_ap[idx] = NULL;
        }
        else
            break;
    }
}

void tm_send_event(tm_input_dev_t* dev, q_bool all)
{
    tm_input_queue_t* q = &dev->input_queue;

    if(all)
    {
        if(q->ap /*&& q->ap->fd > 0*/)
        {
            int i;
            for(i=0; i<q->evt_num; i++)
            {
                q_dbg(Q_DBG_POINT,"%-24s value:%4d",tm_input_evt_str(q->evt[i].type, q->evt[i].code), q->evt[i].value);
                //if(write(q->ap->fd, &q->evt[i], sizeof(tm_input_event_t)) == -1)
                //    q_dbg(Q_ERR,"write %s error",q->ap->evt_path);
            }
        }
    }
    else
    {
        if(q->idx_x != q->idx_y && q->idx_x < q->evt_num && q->idx_y < q->evt_num)
        {
            tm_ap_info_t* ap;

            q_dbg(Q_DBG_POINT,"in  : %d, %d",q->evt[q->idx_x].value, q->evt[q->idx_y].value);
            ap = tm_transfer(&q->evt[q->idx_x].value, &q->evt[q->idx_y].value, dev->panel);
            if(q->ap != ap)
            {

            }
            q_dbg(Q_DBG_POINT,"out : %d, %d",q->evt[q->idx_x].value, q->evt[q->idx_y].value);
            q_dbg(Q_DBG_POINT,"panel id %d -> ap id %d",dev->panel->id, q->ap->id);

            if(q->ap /*&& q->ap->fd > 0*/)
            {
                int i;
                for(i=0; i<q->evt_num; i++)
                {
                    q_dbg(Q_DBG_POINT,"%-24s value:%4d",tm_input_evt_str(q->evt[i].type, q->evt[i].code), q->evt[i].value);
                    //if(write(q->ap->fd, &q->evt[i], sizeof(tm_input_event_t)) == -1)
                    //    q_dbg(Q_ERR,"write %s error",q->ap->evt_path);
                }
            }
        }
    }
    q->evt_num = q->idx_x = q->idx_y = 0;
}

void tm_send_event_to_ap(tm_ap_info_t* ap, tm_input_event_t* evt, uint16_t type, uint16_t code, int val)
{
    evt->type = type;
    evt->code = code;
    evt->value = val;

    if(ap && ap->fd > 0)
    {
        q_dbg(Q_DBG_POINT,"%s",ap->evt_path);
        q_dbg(Q_DBG_POINT,"%-24s value:%4d",tm_input_evt_str(evt->type, evt->code), evt->value);

        if(write(ap->fd, evt, sizeof(tm_input_event_t)) == -1)
            q_dbg(Q_ERR,"write %s error",ap->evt_path);
    }

    tm_input_add_time(&evt->time, 2);
}

void tm_send_single_touch(tm_input_dev_t* dev)
{
    tm_input_queue_t* q = &dev->input_queue;
    tm_input_event_t evt;

    switch(dev->status)
    {
        case TM_INPUT_STATUS_TOUCH:
            dev->status = TM_INPUT_STATUS_PRESS;
            break;

        case TM_INPUT_STATUS_RELEASE:
            tm_input_get_time(&evt.time);
            q->mt[q->slot].p = 0;

            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_PRESSURE, q->mt[q->slot].p);
            tm_send_event_to_ap(q->ap, &evt, EV_KEY, BTN_TOUCH, 0);
            tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);
            dev->status = TM_INPUT_STATUS_IDLE;
            break;

        case TM_INPUT_STATUS_PRESS:
            tm_input_get_time(&evt.time);

            dev->act_ap[0] = tm_transfer(&q->mt[q->slot].x, &q->mt[q->slot].y, dev->panel);

            q->ap = dev->act_ap[0];
            tm_send_event_to_ap(q->ap, &evt, EV_KEY, BTN_TOUCH, 1);
            tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);

            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_X, q->mt[q->slot].x);
            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_Y, q->mt[q->slot].y);
            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_PRESSURE, q->mt[q->slot].p);
            tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);

            dev->status = TM_INPUT_STATUS_DRAG;
            break;

        case TM_INPUT_STATUS_DRAG:
            tm_input_get_time(&evt.time);

            dev->act_ap[0] = tm_transfer(&q->mt[q->slot].x, &q->mt[q->slot].y, dev->panel);

            if(q->ap != dev->act_ap[0])
            {
                tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_PRESSURE, 0);
                tm_send_event_to_ap(q->ap, &evt, EV_KEY, BTN_TOUCH, 0);
                tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);
                q->ap = dev->act_ap[0];
                tm_send_event_to_ap(q->ap, &evt, EV_KEY, BTN_TOUCH, 1);
                tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);
            }

            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_X, q->mt[q->slot].x);
            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_Y, q->mt[q->slot].y);
            tm_send_event_to_ap(q->ap, &evt, EV_ABS, ABS_PRESSURE, q->mt[q->slot].p);
            tm_send_event_to_ap(q->ap, &evt, EV_SYN, SYN_REPORT, 0);
            break;

        default:
            break;
    }
}

void tm_input_parse_single_touch(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    if(dev == NULL)//for test
    {
        dev = list_first_entry(&tm_input.dev_head, tm_input_dev_t, node);
    }

    tm_input_queue_t* q = &dev->input_queue;

    if(tm_input.open == q_false)
        return;

    switch (evt->type)
    {
        case EV_SYN:
            tm_send_single_touch(dev);
            break;

        case EV_KEY:
            if(evt->code == BTN_TOUCH)
            {
                if(evt->value)
                    dev->status = TM_INPUT_STATUS_TOUCH;
                else
                    dev->status = TM_INPUT_STATUS_RELEASE;
            }
            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_X:
                    q->mt[q->slot].x = evt->value;
                    break;
                case ABS_Y:
                    q->mt[q->slot].y = evt->value;
                    break;
                case ABS_PRESSURE:
                    q->mt[q->slot].p = evt->value;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

#if 0
void tm_input_parse_mt_A(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    if(dev == NULL)//for test
    {
        dev = list_first_entry(&tm_input.dev_head, tm_input_dev_t, node);
    }

    tm_input_queue_t* q = &dev->input_queue;

    if(tm_input.open == q_false)
        return;

    memcpy(&q->evt[q->evt_num++], evt, sizeof(tm_input_event_t));

    switch (evt->type)
    {
        case EV_SYN:
            switch(dev->status)
            {
                case TM_INPUT_STATUS_MT_TOUCH:
                    tm_send_event(dev, q_false);
                    dev->status = TM_INPUT_STATUS_MT_A_SYNC;
                    break;

                case TM_INPUT_STATUS_MT_RELEASE:
                    dev->status = TM_INPUT_STATUS_MT_A_SYNC;
                    //break;
                case TM_INPUT_STATUS_MT_A_SYNC:
                    tm_send_event(dev, q_true);
                    break;

                default:
                    break;
            }

            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_MT_POSITION_X:
                    q->idx_x = q->evt_num - 1;
                    dev->status = TM_INPUT_STATUS_MT_TOUCH;
                    break;
                case ABS_MT_POSITION_Y:
                    q->idx_y = q->evt_num - 1;
                    dev->status = TM_INPUT_STATUS_MT_TOUCH;
                    break;
                case ABS_MT_TOUCH_MAJOR:
                    if(evt->value == 0)
                        dev->status = TM_INPUT_STATUS_MT_RELEASE;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}
#endif

void tm_send_multi_touch(tm_input_dev_t* dev)
{

}

void tm_input_parse_multi_touch(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    if(dev == NULL)//for test
    {
        dev = list_first_entry(&tm_input.dev_head, tm_input_dev_t, node);
    }

    tm_input_queue_t* q = &dev->input_queue;

    if(tm_input.open == q_false)
        return;

    q_dbg(Q_DBG_POINT,"%-24s value:%4d",tm_input_evt_str(evt->type, evt->code), evt->value);

    switch (evt->type)
    {
        case EV_SYN:
            tm_send_multi_touch(dev);
            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_MT_POSITION_X:
                    q->idx_x = q->evt_num - 1;
                    q->mt[q->slot].x = evt->value;
                    break;
                case ABS_MT_POSITION_Y:
                    q->idx_y = q->evt_num - 1;
                    q->mt[q->slot].y = evt->value;
                    break;
                case ABS_MT_SLOT:
                    q->slot = evt->value;
                    break;

                case ABS_MT_TRACKING_ID:
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void tm_input_thread_func(void *data)
{
    int ret;
    tm_input_dev_t* dev = (tm_input_dev_t*)data;
    struct timeval tv;
    struct mtdev m_dev;

    memset(&m_dev, 0, sizeof(struct mtdev));

    q_dbg(Q_INFO,"thread run : name : %d %s",dev->panel->name, dev->panel->evt_path);

    if(dev->type != TM_INPUT_TYPE_SINGLE)
    {
        if(mtdev_open(&m_dev, dev->panel->fd))
        {
            q_dbg(Q_ERR,"open mtdev error");
            return;
        }
    }

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

            if(ret > 0 && dev->panel->fd > 0 && FD_ISSET(dev->panel->fd, &dev->evfds))
            {
                char evt[sizeof(tm_input_event_t)] = {0};

                if(dev->type == TM_INPUT_TYPE_SINGLE)
                {
                    ret = q_loop_read(dev->panel->fd, evt, sizeof(tm_input_event_t));
                    if( ret == (ssize_t)sizeof(tm_input_event_t))
                        tm_input_parse_single_touch(dev, (tm_input_event_t*)evt);
                }
                else
                {
                    while(mtdev_get(&m_dev, dev->panel->fd, (tm_input_event_t*)evt, 1) > 0)
                      {
                          tm_input_parse_multi_touch(dev, (tm_input_event_t*)evt);
                      }
                }
            }

            FD_ZERO(&dev->evfds);
            FD_SET(dev->panel->fd, &dev->evfds);
            tv.tv_sec  = 0;
            tv.tv_usec = 500000;
        }
    }

    if(dev->type != TM_INPUT_TYPE_SINGLE)
    {
        mtdev_close(&m_dev);
    }
}
