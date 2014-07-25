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

typedef enum _tm_event_type     tm_event_type_t;
typedef enum _tm_op_event       tm_op_event_t;

typedef struct _tm_fb_param     tm_fb_param_t;
typedef struct _tm_trans_matrix tm_trans_matrix_t;
typedef struct _tm_linear       tm_linear_t;
typedef struct _tm_config       tm_config_t;

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
    int name;
    int16_t max_x;
    int16_t max_y;
	char horizontal;
    char vertical;
    char swap;
};

struct _tm_trans_matrix
{
    int element[CAL_MATRIX_ROW][CAL_MATRIX_COL];
};

struct _tm_config
{
    int name;
    tm_trans_matrix_t trans_matrix;
    int scaling;
    int swap;

    struct{
        int offset;
        int mult;
        int div;
    }pressure;
};


tm_errno_t      tm_mapping_create_handler();
void            tm_mapping_destroy_handler();
tm_errno_t      tm_mapping_update_conf();
tm_errno_t      tm_mapping_transfer(int *x, int *y, tm_config_t* config, tm_fb_param_t*  src_fb, tm_fb_param_t*  dest_fb);
tm_config_t*    tm_mapping_get_config();
tm_fb_param_t*  tm_mapping_get_fb_param();

void tm_mapping_test();

#endif /* TMMAPPING_H_ */
