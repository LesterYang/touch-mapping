/*
 *  tmInput.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : Initialize touch event device and frame buffer device. Receive and 
 *                parse touch event, then redirect events
 */
 
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <linux/input.h>
#include <mtdev.h>
#include <signal.h>


#include "tm.h"

#define BUF_EVT_NUM (64)
#define SELECT_SEC  (0)
#define SELECT_USEC (500000)

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif

typedef struct _tm_input_coord{
    int x;
    int y;
    int p;
    int touch_major;
    int width_major;
    int tracking_id;
}tm_input_coord_t;

typedef struct _tm_input_queue{
    struct{
        tm_ap_info_t*           cur;
        tm_ap_info_t*           last;
    }ap;
    volatile tm_input_status_t  status;
    tm_input_coord_t            cur;
    tm_input_coord_t            last;
    tm_input_coord_t            mt;
    tm_input_coord_t            dup;
}tm_input_queue_t;

typedef struct _tm_input_dev {
    tm_panel_info_t*  panel;
    int               maxfd;
    fd_set            evfds;
    tm_input_type_t   type;

    q_thread*         thread;
    list_head_t	      node;

    unsigned int        threshold; // see THRESHOLD_UNIT
    tm_input_timeval_t  timer;
    tm_input_timespec_t clock;
    tm_input_time_stauts_t timer_status;

    volatile int      slot;
    tm_input_queue_t  input_queue[SLOT_NUM];
}tm_input_dev_t;

typedef struct _tm_input{
    volatile q_bool     open;
    volatile q_bool     suspend;
    list_head_t*        tm_ap_head;
    list_head_t*        tm_pnl_head;

    uint8_t             ap_num;
    uint8_t             dev_num;

    list_head_t         dev_head;
}tm_input_t;

static tm_input_t tm_input;

const char* tm_input_evt_str(int type, int code);
void        tm_input_set_type(tm_input_dev_t* dev);
tm_errno_t  tm_input_init_events(void);
void        tm_input_close_events(void);
void        tm_input_remove_dev(void);

q_bool tm_input_threshold_timeout(tm_input_dev_t* dev);
q_bool tm_input_threshold_clock_timeup(tm_input_dev_t* dev);
void tm_input_send_event(tm_ap_info_t* ap, tm_input_event_t* evt, uint16_t type, uint16_t code, int val);
void tm_input_duplicate_event(tm_panel_info_t* pnl, tm_input_event_t* evt, uint16_t type, uint16_t code, int val);
void tm_input_check_slot(tm_ap_info_t* ap, tm_input_event_t* evt, tm_input_dev_t* dev);
void tm_inpute_duplicate(tm_input_dev_t* dev);
void tm_input_sync_single_touch(tm_input_dev_t* dev);
void tm_input_parse_single_touch(tm_input_dev_t* dev, tm_input_event_t* evt);
void tm_input_sync_multi_touch(tm_input_dev_t* dev);
void tm_input_parse_multi_touch(tm_input_dev_t* dev, tm_input_event_t* evt);
void tm_input_reset_select_time(tm_input_dev_t* dev, tm_input_timeval_t* tv);
void tm_input_thread_func(void *data);


tm_errno_t tm_input_init(list_head_t* ap_head, list_head_t* pnl_head)
{
    tm_errno_t err_no;
    q_assert(ap_head);
    q_assert(pnl_head);

    q_init_head(&tm_input.dev_head);

    tm_input.open        = q_true;
    tm_input.suspend     = q_false;
    tm_input.tm_ap_head	 = ap_head;
    tm_input.tm_pnl_head = pnl_head;
    
    if((err_no = tm_input_init_events()) != TM_ERRNO_SUCCESS)
        return err_no;

    return TM_ERRNO_SUCCESS;
}

void tm_input_deinit()
{
    tm_input.open = q_false;
    tm_input_close_events();
    tm_input_remove_dev();
}

const char* tm_input_evt_str(int type, int code)
{
    if(type == EV_ABS)
    {
        switch(code)
        {
            case ABS_X:             return "type:ABS, code:X";
            case ABS_Y:             return "type:ABS, code:Y";
            case ABS_PRESSURE:      return "type:ABS, code:PRESSURE";
            case ABS_MT_SLOT:       return "type:ABS, code:SLOT";
            case ABS_MT_TOUCH_MAJOR:return "type:ABS, code:TOUCH_MAJOR";
            case ABS_MT_WIDTH_MAJOR:return "type:ABS, code:WIDTH_MAJOR";
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

tm_errno_t tm_input_init_events()
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
            q_dbg(Q_INFO,"Opened ap id %2d -> %s, fd %d",ap->id, ap->evt_path, ap->fd);
    }

    list_for_each_entry(tm_input.tm_pnl_head, panel, node)
    {
        char* threshold;
        
    	dev = (tm_input_dev_t*)q_calloc(sizeof(tm_input_dev_t));

    	if(dev == NULL)
    	    continue;

        if( (threshold = getenv("QSI_TM_THRESHOLD")) == NULL )
            threshold = DEFAULT_THRESHOLD;

    	dev->panel = panel;
        dev->threshold = atoi(threshold);
        tm_input_get_clock_time(&dev->clock);

        q_dbg(Q_DBG_THRESHOLD, "get first clock time: %lu:%lu", dev->clock.tv_sec, dev->clock.tv_nsec);
        q_dbg(Q_DBG_THRESHOLD, "input threshold : %d", dev->threshold);

        q_list_add(&tm_input.dev_head, &dev->node);
    	tm_input.dev_num++;

        if((panel->fd = open (panel->evt_path, O_RDONLY | O_NONBLOCK)) < 0)
        {
            q_dbg(Q_ERR,"Opened %s error",panel->evt_path);
        }
        else
        {
            q_dbg(Q_INFO,"Opened panel id %2d -> %s, fd %d",panel->id, panel->evt_path, panel->fd);
            dev->thread = q_thread_new(tm_input_thread_func, dev);
        }

        if(panel->duplicate && panel->duplicate_evt_path)
        {
            if((panel->duplicate_fd = open (panel->duplicate_evt_path, O_WRONLY)) < 0)
            {
                q_dbg(Q_ERR,"Opened %s error",panel->duplicate_evt_path);
            }
            else
            {
                q_dbg(Q_INFO,"Duplicate panel id %2d -> %s, fd %d",panel->id, panel->duplicate_evt_path, panel->duplicate_fd);
            }
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
            q_close(dev->panel->duplicate_fd);
            dev->panel->fd = -1;
            dev->panel->duplicate_fd = -1;
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
    		q_thread_wait_free(dev->thread);
    		dev->thread = NULL;
    	}

    	q_list_del(&dev->node);
    	q_free(dev);
    }
}

q_bool tm_input_threshold_timeout(tm_input_dev_t* dev)
{
    tm_input_timeval_t now;

    tm_input_get_time(&now);

    if (tm_input_elapsed_time(&now, &dev->timer) > dev->threshold)
        return q_true;
    else
    {
        q_dbg(Q_DBG_THRESHOLD, "ignore event, now time: %lu:%lu", now.tv_sec, now.tv_usec);
        return q_false;
    }
}

q_bool tm_input_threshold_clock_timeup(tm_input_dev_t* dev)
{
    tm_input_timespec_t now;

    tm_input_get_clock_time(&now);

    q_dbg(Q_DBG_THRESHOLD, "old clock time: %lu:%lu", dev->clock.tv_sec, dev->clock.tv_nsec);
    q_dbg(Q_DBG_THRESHOLD, "now clock time: %lu:%lu", now.tv_sec, now.tv_nsec);

    if(now.tv_sec < dev->clock.tv_sec)
        return q_true;

    if (tm_input_elapsed_clock_time(&now, &dev->clock) >= dev->threshold)
    {
        return q_true;
    }
    else
    {
        q_dbg(Q_DBG_THRESHOLD, "--- ignore event ---");
        return q_false;
    }
}


void tm_input_send_event(tm_ap_info_t* ap, tm_input_event_t* evt, uint16_t type, uint16_t code, int val)
{
    q_assert(ap);

    evt->type = type;
    evt->code = code;
    evt->value = val;

    if(ap->fd > 0)
    {
        q_dbg(Q_DBG_POINT,"send to %s, event :",ap->evt_path);
        q_dbg(Q_DBG_POINT,"send %-24s value:%4d",tm_input_evt_str(evt->type, evt->code), evt->value);

        if(write(ap->fd, evt, sizeof(tm_input_event_t)) == -1)
            q_dbg(Q_ERR,"write %s error",ap->evt_path);
    }

    tm_input_add_time(&evt->time, 5);
}

void tm_input_duplicate_event(tm_panel_info_t* pnl, tm_input_event_t* evt, uint16_t type, uint16_t code, int val)
{
    q_assert(pnl);

    evt->type = type;
    evt->code = code;
    evt->value = val;

    if(pnl->duplicate_fd >= 0)
    {
        if(write(pnl->duplicate_fd, evt, sizeof(tm_input_event_t)) == -1)
            q_dbg(Q_ERR,"write %s error",pnl->duplicate_evt_path);
    }

    tm_input_add_time(&evt->time, 5);
}


void tm_input_check_slot(tm_ap_info_t* ap, tm_input_event_t* evt, tm_input_dev_t* dev)
{
    if (dev->slot != ap->slot) 
    {
        ap->slot = dev->slot;
        q_dbg(Q_DBG_SEND_MULTI,"send slot %d",ap->slot);
        tm_input_send_event(ap, evt, EV_ABS, ABS_MT_SLOT, ap->slot);
    }
}

void tm_inpute_duplicate(tm_input_dev_t* dev)
{
    tm_panel_info_t* pnl = dev->panel;
    tm_input_queue_t* q = &dev->input_queue[dev->slot];
    tm_input_event_t evt;
    static int last_slot;

    if(pnl->duplicate == q_false)
        return;
        
    q->dup.x = q->mt.x;
    q->dup.y = q->mt.y;
    q->dup.touch_major = q->mt.touch_major;
    q->dup.width_major = q->mt.width_major;
    q->dup.tracking_id = q->mt.tracking_id;

    tm_input_get_time(&evt.time);

    switch(q->status)
    {
        case TM_INPUT_STATUS_PRESS:
            tm_duplicate_transfer(&q->dup.x, &q->dup.y, pnl);
            tm_input_duplicate_event(pnl, &evt, EV_KEY, BTN_TOUCH, 1);
            tm_input_duplicate_event(pnl, &evt, EV_SYN, SYN_REPORT, 0);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_X, q->dup.x);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_Y, q->dup.y);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_PRESSURE, q->dup.p);
            break;
            
        case TM_INPUT_STATUS_DRAG:
            tm_duplicate_transfer(&q->dup.x, &q->dup.y, pnl);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_X, q->dup.x);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_Y, q->dup.y);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_PRESSURE, q->dup.p);
            break;
            
        case TM_INPUT_STATUS_RELEASE:
            q->dup.p = q->mt.p = 0;
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_PRESSURE, q->dup.p);
            tm_input_duplicate_event(pnl, &evt, EV_KEY, BTN_TOUCH, 0);
            break;
            
        case TM_INPUT_STATUS_MT_PRESS:
            tm_duplicate_transfer(&q->dup.x, &q->dup.y, pnl);
            if(last_slot != dev->slot)
            {
                last_slot = dev->slot;
                tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_SLOT, dev->slot);
            } 
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_TRACKING_ID, q->dup.tracking_id);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_POSITION_X, q->dup.x);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_POSITION_Y, q->dup.y);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_TOUCH_MAJOR, q->dup.touch_major);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_WIDTH_MAJOR, q->dup.width_major);
            break;
            
        case TM_INPUT_STATUS_MT_DRAG:
            tm_duplicate_transfer(&q->dup.x, &q->dup.y, pnl);
            if(last_slot != dev->slot)
            {
                last_slot = dev->slot;
                tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_SLOT, dev->slot);
            } 
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_POSITION_X, q->dup.x);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_POSITION_Y, q->dup.y);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_TOUCH_MAJOR, q->dup.touch_major);
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_WIDTH_MAJOR, q->dup.width_major);
            break;
            
        case TM_INPUT_STATUS_MT_RELEASE:
            q->dup.touch_major = q->mt.touch_major = 0;
            if(last_slot != dev->slot)
            {
                last_slot = dev->slot;
                tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_SLOT, dev->slot);
            } 
            tm_input_duplicate_event(pnl, &evt, EV_ABS, ABS_MT_TRACKING_ID, q->dup.tracking_id);
            break;
            
        default:
            return;
    }

    tm_input_duplicate_event(pnl, &evt, EV_SYN, SYN_REPORT, 0);
}


void tm_input_sync_single_touch(tm_input_dev_t* dev)
{
    tm_input_queue_t* q = &dev->input_queue[0];
    tm_input_event_t evt;
    
    switch(q->status)
    {
        case TM_INPUT_STATUS_TOUCH:
            q->status = TM_INPUT_STATUS_PRESS;
            break;

        case TM_INPUT_STATUS_RELEASE:
            tm_input_get_time(&evt.time);
            q->cur.p = q->mt.p = 0;
            q->ap.cur = q->ap.last;

            if(!q->ap.cur)
                return;

            tm_inpute_duplicate(dev);

            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_PRESSURE, q->cur.p);
            tm_input_send_event(q->ap.cur, &evt, EV_KEY, BTN_TOUCH, 0);
            tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);
            q->status = TM_INPUT_STATUS_IDLE;
            tm_input_get_clock_time(&dev->clock);
            break;

        case TM_INPUT_STATUS_PRESS:
            tm_input_get_time(&evt.time);
            q->cur.x = q->mt.x;
            q->cur.y = q->mt.y;
            q->cur.p = q->mt.p;

            q->ap.cur = tm_transfer(&q->cur.x, &q->cur.y, dev->panel);

            if(q->ap.cur)
            {
                q->ap.last = q->ap.cur;
            }
            else
            {
                return;
            }

            if(q->ap.cur->threshold && !tm_input_threshold_clock_timeup(dev))
            {
                q->status = TM_INPUT_STATUS_IDLE;
                return;
            }

            tm_inpute_duplicate(dev);

            q_dbg(Q_INFO, "press event, %s to %s\n",dev->panel->evt_path, q->ap.cur->evt_path);

            tm_input_send_event(q->ap.cur, &evt, EV_KEY, BTN_TOUCH, 1);
            tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);

            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_X, q->cur.x);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_Y, q->cur.y);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_PRESSURE, q->cur.p);
            tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);

            q->status = TM_INPUT_STATUS_DRAG;
            break;

        case TM_INPUT_STATUS_DRAG:
            tm_input_get_time(&evt.time);
            q->cur.x = q->mt.x;
            q->cur.y = q->mt.y;
            q->cur.p = q->mt.p;

            q->ap.cur = tm_transfer(&q->cur.x, &q->cur.y, dev->panel);

            if(!q->ap.cur)
                return;

            tm_inpute_duplicate(dev);
        
            if(q->ap.cur != q->ap.last)
            {
                tm_input_send_event(q->ap.last, &evt, EV_ABS, ABS_PRESSURE, 0);
                tm_input_send_event(q->ap.last, &evt, EV_KEY, BTN_TOUCH, 0);
                tm_input_send_event(q->ap.last, &evt, EV_SYN, SYN_REPORT, 0);
                q->ap.last = q->ap.cur;
                tm_input_send_event(q->ap.cur, &evt, EV_KEY, BTN_TOUCH, 1);
                tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);
            }
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_X, q->cur.x);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_Y, q->cur.y);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_PRESSURE, q->cur.p);
            tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);
            break;

        case TM_INPUT_STATUS_IDLE:
            //if(tm_input_threshold_timeout(dev) && q->mt.p)
            //    q->status = TM_INPUT_STATUS_PRESS;
            break;
        default:
            break;
    }
}

void tm_input_parse_single_touch(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    tm_input_queue_t* q = &dev->input_queue[0];

    if(tm_input.open == q_false)
        return;

    switch (evt->type)
    {
        case EV_SYN:
            tm_input_sync_single_touch(dev);
            break;

        case EV_KEY:
            if(evt->code == BTN_TOUCH)
            {      
                if(evt->value)
                {
                    q->status = TM_INPUT_STATUS_TOUCH;
                }
                else if(q->status == TM_INPUT_STATUS_PRESS)
                {
                    // ignore release, because haven't press yet
                    q->status = TM_INPUT_STATUS_IDLE;
                    tm_input_get_clock_time(&dev->clock);
                }
                else if(q->status != TM_INPUT_STATUS_IDLE)
                {
                    q->status = TM_INPUT_STATUS_RELEASE;
                }
            }
            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_X:
                    q->mt.x = evt->value;
                   break;
                case ABS_Y:
                    q->mt.y = evt->value;
                    break;
                case ABS_PRESSURE:
                    q->mt.p = evt->value;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}


void tm_input_sync_multi_touch(tm_input_dev_t* dev)
{
    tm_input_queue_t* q = &dev->input_queue[dev->slot];
    tm_input_event_t evt; 

    tm_input_get_time(&evt.time);

    q_dbg(Q_DBG_MULTI_POINT,"x y : %4d %4d",q->mt.x, q->mt.y);
    q_dbg(Q_DBG_MULTI_POINT,"maj : %4d %4d", q->mt.touch_major, q->mt.width_major);
    q_dbg(Q_DBG_MULTI_POINT,"s id: %4d %4d", dev->slot, q->mt.tracking_id);
    
    switch(q->status)
    {
        case TM_INPUT_STATUS_MT_PRESS:
            q->cur.x = q->mt.x;
            q->cur.y = q->mt.y;
            q->cur.touch_major = q->mt.touch_major;
            q->cur.width_major = q->mt.width_major;
            q->cur.tracking_id = q->mt.tracking_id;

            q->ap.cur = tm_transfer(&q->cur.x, &q->cur.y, dev->panel);

            if(q->ap.cur)
            {
                q->ap.last = q->ap.cur;
            }
            else
            {
                return;
            }

            q_dbg(Q_INFO, "press event, %s to %s\n",dev->panel->evt_path, q->ap.cur->evt_path);

            tm_inpute_duplicate(dev);
            tm_input_check_slot(q->ap.cur, &evt, dev);

            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_TRACKING_ID, q->cur.tracking_id);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_POSITION_X, q->cur.x);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_POSITION_Y, q->cur.y);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_TOUCH_MAJOR, q->cur.touch_major);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_WIDTH_MAJOR, q->cur.width_major);

            q_dbg(Q_DBG_SEND_MULTI,"send id: %d",q->cur.tracking_id);
            q_dbg(Q_DBG_SEND_MULTI,"send xy: %4d %4d",q->cur.x, q->cur.y);
            
            q->status = TM_INPUT_STATUS_MT_DRAG;
            break;

        case TM_INPUT_STATUS_MT_RELEASE:
            q->cur.touch_major = q->mt.touch_major = 0;
            q->cur.tracking_id = q->mt.tracking_id;
            q->ap.cur = q->ap.last;
            
            if(!q->ap.cur)
                return;

            tm_inpute_duplicate(dev);
            tm_input_check_slot(q->ap.cur, &evt, dev);
            
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_TRACKING_ID, q->cur.tracking_id);
            q_dbg(Q_DBG_SEND_MULTI,"send id: %d",q->cur.tracking_id);
            q->status = TM_INPUT_STATUS_IDLE;
            break;

        case TM_INPUT_STATUS_MT_DRAG:
            q->cur.x = q->mt.x;
            q->cur.y = q->mt.y;
            q->cur.touch_major = q->mt.touch_major;
            
            q->ap.cur = tm_transfer(&q->cur.x, &q->cur.y, dev->panel);

            if(!q->ap.cur)
                return;

            tm_inpute_duplicate(dev);
            
            if(q->ap.last != q->ap.cur)
            {    
                tm_input_check_slot(q->ap.last, &evt, dev);
                tm_input_send_event(q->ap.last, &evt, EV_ABS, ABS_MT_TRACKING_ID, -1);
                q_dbg(Q_DBG_SEND_MULTI,"send id: -1");
                tm_input_check_slot(q->ap.cur, &evt, dev);
                tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_TRACKING_ID, q->cur.tracking_id);
                q_dbg(Q_DBG_SEND_MULTI,"send id: %d",q->cur.tracking_id);
            }
            else
            {
                tm_input_check_slot(q->ap.cur, &evt, dev);
            }

            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_POSITION_X, q->cur.x);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_POSITION_Y, q->cur.y);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_TOUCH_MAJOR, q->cur.touch_major);
            tm_input_send_event(q->ap.cur, &evt, EV_ABS, ABS_MT_WIDTH_MAJOR, q->cur.width_major);
            q_dbg(Q_DBG_SEND_MULTI,"send xy: %4d %4d",q->cur.x, q->cur.y);
            q_dbg(Q_DBG_SEND_MULTI,"send ma: %4d %4d",q->cur.touch_major, q->cur.width_major);

            break;

        default:
            return;
    }

    q_dbg(Q_DBG_SEND_MULTI,"------ Sync ------");
    tm_input_send_event(q->ap.cur, &evt, EV_SYN, SYN_REPORT, 0);
    return;
}

void tm_input_parse_multi_touch(tm_input_dev_t* dev, tm_input_event_t* evt)
{
    tm_input_queue_t* q = &dev->input_queue[dev->slot];

    if(tm_input.open == q_false)
        return;

    q_dbg(Q_DBG_RECV_MULTI,"%-24s value:%4d",tm_input_evt_str(evt->type, evt->code), evt->value);

    switch (evt->type)
    {
        case EV_SYN:
            tm_input_sync_multi_touch(dev);
            break;

        case EV_ABS:
            switch (evt->code)
            {
                case ABS_MT_POSITION_X:
                    q->mt.x = evt->value;
                    break;
                case ABS_MT_POSITION_Y:
                    q->mt.y = evt->value;
                    break;
                case ABS_MT_TOUCH_MAJOR:
                    q->mt.touch_major = evt->value;
                    break;
                case ABS_MT_WIDTH_MAJOR:
                    q->mt.width_major = evt->value;
                    break;
                case ABS_MT_SLOT:
                    dev->slot = evt->value;
                    break;

                case ABS_MT_TRACKING_ID:
                    if((q->mt.tracking_id = evt->value) == -1)
                        q->status = TM_INPUT_STATUS_MT_RELEASE;
                    else
                        q->status = TM_INPUT_STATUS_MT_PRESS;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void tm_input_reset_select_time(tm_input_dev_t* dev, tm_input_timeval_t* tv)
{
    FD_ZERO(&dev->evfds);
    FD_SET(dev->panel->fd, &dev->evfds);
    tv->tv_sec  = SELECT_SEC;
    tv->tv_usec = SELECT_USEC;
}

void tm_input_thread_func(void *data)
{
    int ret = 0;
    tm_input_dev_t* dev = (tm_input_dev_t*)data;
    tm_input_timeval_t tv;
    struct mtdev m_dev;

    memset(&m_dev, 0, sizeof(struct mtdev));

    q_dbg(Q_DBG,"thread run : name : %d %s",dev->panel->id, dev->panel->evt_path);

    if(dev->type != TM_INPUT_TYPE_SINGLE)
    {
        if(mtdev_open(&m_dev, dev->panel->fd))
        {
            q_dbg(Q_ERR,"open mtdev error");
            return;
        }
    }

    if(dev->panel->fd > dev->maxfd)
        dev->maxfd = dev->panel->fd;

    while(tm_input.open)
    {
        do{
            q_thread_cancellation_point();

            if(ret > 0 && dev->panel->fd > 0 && FD_ISSET(dev->panel->fd, &dev->evfds))
            {
                char evt[sizeof(tm_input_event_t)] = {0};

                if(dev->type == TM_INPUT_TYPE_SINGLE)
                {
                    ret = q_read(dev->panel->fd, evt, sizeof(tm_input_event_t));
                    if( ret == (ssize_t)sizeof(tm_input_event_t) && !tm_input.suspend)
                        tm_input_parse_single_touch(dev, (tm_input_event_t*)evt);
                }
                else
                {
                    while(mtdev_get(&m_dev, dev->panel->fd, (tm_input_event_t*)evt, 1) > 0)
                    {
                        if(!tm_input.suspend)
                            tm_input_parse_multi_touch(dev, (tm_input_event_t*)evt);
                    }
                }
            }
            tm_input_reset_select_time(dev, &tv);
        }while( (ret = select((dev->maxfd)+1,  &dev->evfds, NULL, NULL, &tv)) >= 0);
    }

    if(dev->type != TM_INPUT_TYPE_SINGLE)
    {
        mtdev_close(&m_dev);
    }
}

