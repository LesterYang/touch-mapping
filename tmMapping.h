/*
 * tmMapping.h
 *
 *  Created on: Aug 1, 2014
 *      Author: lester
 */

#ifndef TMMAPPING_H_
#define TMMAPPING_H_

#include <stdio.h>
#include <stdint.h>
#include <linux/input.h>
#include "qUtils.h"
#include "tmError.h"

//#define QSI_TM_CONF         "/mnt/hgfs/Win_7/workspace-cpp2/touch-mapping/qsi_tm.conf"
//#define QSI_TM_CONF           "/root/script/qsi_tm.conf"
//#define QSI_TM_CONF         "/home/lester/Git/touch-mapping/qsi_tm.conf"
#define QSI_TM_CONF         "/etc/qsi_tm.conf"
#define AP_CONF             "ap_info"
#define PNL_CONF            "pnl_info"
#define CAL_CONF            "cal_conf"
#define SIZE_CONF           "native_size"
#define AT_CONF             "single_touch"
#define MT_CONF             "multi_touch"

#define BUF_SIZE            (256)
#define MULTIPLE            (4096)
#define MAX_QUEUE           (512)

#define CAL_MATRIX_ROW      (2)
#define CAL_MATRIX_COL      (3)

#define JITTER_BOUNDARY     (3)

#define dejitter_boundary(pos, max, delta) (pos < 0 && pos > -(delta)) ? 0 : (pos > max && pos < max + delta) ? max : pos

#define FB_LEN_X(fb) (fb.abs_st_x - fb.abs_end_x)
#define FB_LEN_Y(fb) (fb.abs_st_y - fb.abs_end_y)

typedef enum _tm_event_type             tm_event_type_t;
typedef enum _tm_op_event               tm_op_event_t;

typedef struct _tm_native_size_param    tm_native_size_param_t;
typedef struct _tm_fb_param             tm_fb_param_t;
typedef struct _tm_trans_matrix         tm_trans_matrix_t;
typedef struct _tm_calibrate            tm_calibrate_t;
typedef struct _tm_config               tm_config_t;

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

struct _tm_native_size_param
{
    int     id;
    int16_t max_x;
    int16_t max_y;

	list_head_t	   node;
};

struct _tm_trans_matrix
{
    int element[CAL_MATRIX_ROW][CAL_MATRIX_COL];
};

struct _tm_calibrate
{
    int                 id;
    tm_trans_matrix_t   trans_matrix;
    int                 scaling;

    struct{
        int offset;
        int mult;
        int div;
    }pressure;

	list_head_t	    node;
};

struct _tm_fb_param // relative proportion to native size
{
    int st_x;
    int st_y;
    int w;
    int h;

    int abs_st_x;
    int abs_st_y;
    int abs_end_x;
    int abs_end_y;
    
    char horizontal;
    char vertical;
    char swap;
};

struct _tm_config
{
    tm_native_size_param_t  head_size;
    tm_calibrate_t          head_cal;
    uint8_t                 native_size_num;
    uint8_t                 calibrate_num;
	
	list_head_t	   			native_size_head;
	list_head_t	   			calibrate_head;
};

void 			tm_mapping_print_conf(list_head_t* ap_head, list_head_t* pnl_head);

tm_errno_t 		tm_mapping_update_conf(list_head_t* ap_head, list_head_t* pnl_head);
void            tm_mapping_remove_conf(list_head_t* ap_head, list_head_t* pnl_head);
tm_errno_t      tm_mapping_calibrate_conf(void);
tm_errno_t      tm_mapping_native_size_conf(void);
tm_errno_t 	    tm_mapping_pnl_conf(list_head_t* pnl_head);
tm_errno_t 		tm_mapping_ap_conf(list_head_t* ap_head);
tm_errno_t  	tm_mapping_create_handler(list_head_t* ap_head, list_head_t* pnl_head);
void            tm_mapping_destroy_handler(list_head_t* ap_head, list_head_t* pnl_head);

tm_calibrate_t*          tm_mapping_get_calibrate_param(int id);
tm_native_size_param_t*  tm_mapping_get_native_size_param(int id);
void tm_mapping_test();

#endif /* TMMAPPING_H_ */
