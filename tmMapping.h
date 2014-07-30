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
#include "qUtils.h"
#include "tmError.h"

#define QSI_TM_CONF         "/mnt/hgfs/Win_7/workspace-cpp2/touch-mapping/qsi_tm.conf"
#define BUF_SIZE            (256)
#define MULTIPLE            (4096)
#define MAX_QUEUE           (512)

#define CAL_MATRIX_ROW      (2)
#define CAL_MATRIX_COL      (3)

#define __tm_mapping_list_add(head, new)            \
({                                                  \
    new->next  = (head->next) ? head->next : NULL;  \
    head->next = new;                               \
})

typedef enum _tm_event_type             tm_event_type_t;
typedef enum _tm_op_event               tm_op_event_t;

typedef struct _tm_native_size_param    tm_native_size_param_t;
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
    tm_fb_t name;
    int16_t max_x;
    int16_t max_y;
	char    horizontal;
    char    vertical;
    char    swap;

    tm_native_size_param_t* next;
};

struct _tm_trans_matrix
{
    int element[CAL_MATRIX_ROW][CAL_MATRIX_COL];
};

struct _tm_calibrate
{
    tm_panel_t          name;
    tm_trans_matrix_t   trans_matrix;
    int                 scaling;

    struct{
        int offset;
        int mult;
        int div;
    }pressure;

    tm_calibrate_t* next;
};

struct _tm_config
{
    tm_native_size_param_t  head_size;
    tm_calibrate_t          head_cal;
};

void            tm_mapping_init_config_list(void);
void            tm_mapping_add_native_size_praram(tm_native_size_param_t* size);
void            tm_mapping_add_calibrate_praram(tm_calibrate_t* cal);
tm_errno_t      tm_mapping_create_handler(void);
void            tm_mapping_destroy_handler(void);
tm_errno_t      tm_mapping_update_conf(void);
void            tm_mapping_calibrate_conf(void);
void            tm_mapping_native_size_conf(void);
tm_errno_t      tm_mapping_transfer(int *x, int *y, tm_config_t* config, tm_fb_param_t*  src_fb, tm_fb_param_t*  dest_fb);

tm_calibrate_t*          tm_mapping_get_calibrate_param(tm_panel_t name);
tm_native_size_param_t*  tm_mapping_get_native_size_param(tm_fb_t name);

void tm_mapping_test();

#endif /* TMMAPPING_H_ */
