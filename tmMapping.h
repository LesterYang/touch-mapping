/*
 * tmMapping.h
 *
 *  Created on: Jul 18, 2014
 *      Author: root
 */

#ifndef TMMAPPING_H_
#define TMMAPPING_H_

#include <stdio.h>
#include <stdint.h>
#include <linux/input.h>
#include "_tm.h"

#define QSI_TM_CONF         "/mnt/hgfs/Win_7/workspace-cpp2/touch-mapping/qsi_tm.conf"
#define BUF_SIZE            (256)
#define MULTIPLE            (4096)
#define MAX_QUEUE           (512)

typedef enum _tm_ap             tm_ap_t;
typedef enum _tm_panel          tm_panel_t;
typedef enum _tm_event_type     tm_event_type_t;
typedef enum _tm_op_event       tm_op_event_t;

typedef struct input_event      tm_input_event_t;
typedef struct _tm_event_info   tm_event_info_t;
typedef struct _tm_panel_info	tm_panel_info_t;
typedef struct _tm_fb_param     tm_fb_param_t;
typedef struct _tm_trans_matrix tm_trans_matrix_t;
typedef struct _tm_config       tm_config_t;

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

enum _tm_event_type{
    TM_EVENT_TYPE_SOURCE,
    TM_EVENT_TYPE_TARGET
};

enum _tm_op_event{
    TM_OP_EVENT_NONE,
    TM_OP_EVENT_START,
    TM_OP_EVENT_TRANS_X,
    TM_OP_EVENT_TRANS_Y,
    TM_OP_EVENT_MT_END,
    TM_OP_EVENT_END
};

struct _tm_fb_param
{
    int16_t x;
    int16_t y;
	char horizontal;
    char vertical;
    char swap;
    char used;
};

struct _tm_trans_matrix
{
    element[3][2];
};


struct _tm_event_info
{
	tm_ap_t         ap;
    const char*     event_input_path;
	int             fd_in;
	const char*     event_onput_path;
	int             fd_out;
    tm_fb_param_t*  fb_param;           // Global array in tmMap.c,  tm_fb_param_t fb_param[TM_PANEL_NUM]
};

struct _tm_panel_info
{
	tm_panel_t          panel;
	tm_event_info_t*    current;        // Global array in tm.c, tm_event_info_t event[TM_AP_NUM]
	tm_trans_matrix_t*  trans_matrix;   // Global array in tmMap.c,  tm_trans_matrix_t cal[TM_PANEL_NUM]
	tm_panel_info_t*    dest_panel;
}



const char* tm_err_str(tm_errno_t no);
tm_errno_t  tm_mapping_create_handler(tm_handler_t** p_tm);
void        tm_mapping_destroy_handler(tm_handler_t* tm);
tm_errno_t  tm_mapping_update_conf();      // update tm_trans_matrix_t cal[TM_PANEL_NUM]

tm_errno_t  tm_mapping_transfer(int16_t *x, int16_t *y, tm_panel_info_t* src);


#endif /* TMMAPPING_H_ */
