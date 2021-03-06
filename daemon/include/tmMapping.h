#ifndef TMMAPPING_H_
#define TMMAPPING_H_

#include <stdio.h>
#include <stdint.h>
#include <linux/input.h>
#include "qUtils.h"
#include "tmError.h"

#define TM_PATH_LEN         (1024)
#define LST_TM_CFG          "/home/lst_tm/lst_tm.conf"
#define AP_CFG              "ap_info"
#define PNL_CFG             "pnl_info"
#define CAL_CFG             "cal_conf"
#define FB_CFG              "fb_info"
#define COPY_CFG            "duplicate"


#define FB_CFG_DEV          "/dev/fb"
#define FB_CFG_AUTO_DEV     "auto"
#define FB_CFG_DEFAULT_X    (800)
#define FB_CFG_DEFAULT_Y    (480)

#define BUF_SIZE            (256)
#define MULTIPLE            (4096)
#define MAX_QUEUE           (512)

#define CAL_MATRIX_ROW      (2)
#define CAL_MATRIX_COL      (3)

#define JITTER_BOUNDARY     (3)

#define dejitter_boundary(pos, max, delta)  (pos < 0 && pos > -(delta)) ? 0 :               \
                                            (pos > max && pos < max + delta) ? max : pos

#define FB_LEN_X(fb) (fb.abs_begin_x - fb.abs_end_x)
#define FB_LEN_Y(fb) (fb.abs_begin_y - fb.abs_end_y)
#define FB_MAP_X(x, p)                                          \
    ( p->from_ap.abs_begin_x                                    \
      + ((x - p->to_pnl.abs_begin_x) * FB_LEN_X(p->from_ap))    \
      / FB_LEN_X(p->to_pnl))
#define FB_MAP_Y(y, p)                                          \
    ( p->from_ap.abs_begin_y                                    \
      + ((y - p->to_pnl.abs_begin_y) * FB_LEN_Y(p->from_ap))    \
      / FB_LEN_Y(p->to_pnl))


typedef struct _tm_native_size_param    tm_native_size_param_t;
typedef struct _tm_fb_param             tm_fb_param_t;
typedef struct _tm_trans_matrix         tm_trans_matrix_t;
typedef struct _tm_calibrate            tm_calibrate_t;
typedef struct _tm_config               tm_config_t;

struct _tm_native_size_param
{
    int     id;
    int16_t max_x;
    int16_t max_y;
    const char* fb_path;
    list_head_t node;
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

    list_head_t         node;
};

struct _tm_fb_param // relative proportion to native size
{
    // percent
    int begin_x;
    int begin_y;
    int w;
    int h;

    // absolute position
    int abs_begin_x;
    int abs_begin_y;
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
	
    list_head_t             native_size_head;
    list_head_t             calibrate_head;
};

tm_errno_t tm_mapping_create_handler(list_head_t* ap_head, list_head_t* pnl_head);
void       tm_mapping_destroy_handler(list_head_t* ap_head, list_head_t* pnl_head);
bool       tm_mapping_native_size_is_const(tm_native_size_param_t* size);

void tm_mapping_print_config(list_head_t* ap_head, list_head_t* pnl_head);
void tm_mapping_print_cal_info(void);
void tm_mapping_print_size_info(void);
void tm_mapping_print_ap_info(list_head_t* ap_head);
void tm_mapping_print_panel_info(list_head_t* pnl_head);

#endif /* TMMAPPING_H_ */
