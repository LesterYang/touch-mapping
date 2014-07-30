/*
 * _tm.h
 *
 *  Created on: Jul 18, 2014
 *      Author: root
 */

#ifndef TM_H_
#define TM_H_

#include <stdint.h>
#include "qUtils.h"
#include "tmError.h"
#include "tmMapping.h"

#define TM_HDR_LEN    (2)
#define TM_HDR_0      (0xff)
#define TM_HDR_1      (0x55)

typedef enum _tm_status     tm_status_t;

typedef enum _tm_ap         tm_ap_t;
typedef enum _tm_fb         tm_fb_t;
typedef enum _tm_panel      tm_panel_t;

typedef struct _tm_ap_info      tm_ap_info_t;
typedef struct _tm_panel_info   tm_panel_info_t;


enum _tm_status{
    TM_STATUS_NONE,
    TM_STATUS_INIT,
    TM_STATUS_IPC_INIT,
    TM_STATUS_RUNNING,
    TM_STATUS_DEINIT,
    TM_STATUS_ERROR,
    TM_STATUS_EXIT
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
    tm_ap_t         name;
    const char*     event_path;
    int             fd;
    tm_fb_param_t*  fb_param;           // Global array in tmMap.c,  tm_fb_param_t fb_param[TM_PANEL_NUM]
    q_mutex*        mutex;
};

struct _tm_panel_info                   // Global array in tm.c, tm_panel_info_t panel[TM_PANEL_NUM]
{
    tm_panel_t      name;
    const char*     event_path;
    int             fd;
    tm_ap_info_t*   ap;                 // Global array in tm.c, tm_event_info_t event[TM_AP_NUM]
    tm_config_t*    cal_param;          // Global array in tmMap.c,  tm_config_t cal[TM_PANEL_NUM]
    tm_fb_param_t*  fb_param;
};


void        tm_set_default_direction();
tm_errno_t  tm_init(void);
void        tm_deinit(void);
void        tm_bind_panel_ap(tm_panel_t panel, tm_panel_t ap);
void        tm_set_status(tm_status_t status);
void        tm_bind_status(tm_status_t* status);
void        tm_bind_param();
tm_errno_t  tm_transfer(int *x, int *y, tm_panel_info_t* panel);

#endif /* TM_H_ */
