/*
 * _tm.h
 *
 *  Created on: Aug 1, 2014
 *      Author: lester
 */

#ifndef TM_H_
#define TM_H_

#include <stdint.h>
#include "qUtils.h"
#include "tmError.h"
#include "tmMapping.h"
#include "tmInput.h"

#define TM_VERSION "1.0"

#define TM_HDR_LEN    (2)
#define TM_HDR_0      (0xff)
#define TM_HDR_1      (0x55)

// IPC command
#define IPC_CMD_SET_MAP  (0xa0)
#define IPC_CMD_CLR_MAP  (0xa1)



#define IPC_SET_MAP_LEN  (10)
#define IPC_CLR_MAP_LEN  (1)

#define __tm_list_add(head, new)                    \
({                                                  \
    new->next  = (head.next) ? head.next : NULL;    \
    head.next = new;                                \
})

#define tm_point_is_in_range(fb, x, y)              \
(!( x < (fb)->abs_st_x    ||                        \
	x > (fb)->abs_end_x   ||                        \
	y < (fb)->abs_st_y    ||                        \
	y > (fb)->abs_end_y   )                         \
)


typedef enum _tm_status     tm_status_t;
typedef enum _tm_ipc_status tm_ipc_status_t;

typedef enum _tm_ap         tm_ap_t;
typedef enum _tm_fb         tm_fb_t;
typedef enum _tm_panel      tm_panel_t;

typedef struct _tm_ap_info      tm_ap_info_t;
typedef struct _tm_panel_info   tm_panel_info_t;
typedef struct _tm_display      tm_display_t;


enum _tm_status{
    TM_STATUS_NONE,
    TM_STATUS_INIT,
    TM_STATUS_IPC_INIT,
    TM_STATUS_RUNNING,
    TM_STATUS_DEINIT,
	TM_STATUS_REINIT,
    TM_STATUS_ERROR,
    TM_STATUS_EXIT
};

enum _tm_ipc_status{
    TM_IPC_STATUS_NONE,
    TM_IPC_STATUS_INIT,
    TM_IPC_STATUS_RUN,
    TM_IPC_STATUS_SETTING,
    TM_IPC_STATUS_ERROR,
    TM_IPC_STATUS_EXIT
};

enum _tm_ap{
    TM_AP_QSI,
    TM_AP_QSI_L,
    TM_AP_QSI_R,
    TM_AP_NAVI,
    TM_AP_MONITOR,

    TM_AP_MAX,
    TM_AP_NUM = TM_AP_MAX,
    TM_AP_NONE = -1
};

enum _tm_panel{
    TM_PANEL_FRONT,
    TM_PANEL_LEFT,
    TM_PANEL_RIGHT,

    TM_PANEL_MAX,
    TM_PANEL_NUM = TM_PANEL_MAX,
    TM_PANEL_NONE = -1
};

enum _tm_fb{
    TM_FB_800_480_0_0_0,
    TM_FB_1000_600_0_0_0,

    TM_FB_MAX,
    TM_FB_NUM = TM_FB_MAX,
    TM_FB_NONE = -1
};

struct _tm_ap_info
{
	union{
    tm_ap_t                  name;
    int                      id;
	};

    const char*              evt_path;
    int                      fd;
    tm_input_type_t          touch_type;
    tm_native_size_param_t*  native_size;
	
	list_head_t	             node;
    q_mutex*                 mutex;
};

struct _tm_display
{
    tm_ap_info_t* ap;
    tm_fb_param_t from;
    tm_fb_param_t to;
	
	list_head_t	   node;
    tm_display_t* next;
};

struct _tm_panel_info
{
	union{
    tm_panel_t      name;
    int             id;
	};

    const char*     evt_path;
    int             fd;
    int             link_num;

    tm_calibrate_t*          cal_param;
    tm_native_size_param_t*  native_size;

	list_head_t	   node;
	list_head_t	   display_head;
    q_mutex*       mutex;
    q_queue*       queue;
};

tm_errno_t  tm_set_fb_param(tm_fb_param_t* fb, int start_x, int start_y, int per_width, int per_high);

void 		_tm_remove_display_conf(tm_panel_info_t* panel);
void 		tm_remove_display_conf(void);

void        tm_fill_up_fb_conf(tm_fb_param_t* fb, tm_native_size_param_t* native);
void        tm_set_default_display();
tm_ap_info_t* tm_mapping_get_default_ap(int panel_id);

tm_errno_t  tm_init(void);
void        tm_deinit(void);
void        tm_set_status(tm_status_t status);
void        tm_bind_status(tm_status_t* status);

void        tm_set_map(unsigned int len, unsigned char *msg);
void        tm_clear_map(unsigned int len, unsigned char *msg);

tm_ap_info_t*   	tm_mapping_get_ap_info(int id);
tm_panel_info_t*	tm_mapping_get_panel_info(int id);

tm_ap_info_t* tm_match_ap(int x, int y, tm_panel_info_t* panel);
tm_display_t* tm_match_display(int x, int y, tm_panel_info_t* panel);

tm_ap_info_t* tm_transfer(int *x, int *y, tm_panel_info_t* panel);

void tm_dbg_print_conf();

#endif /* TM_H_ */
