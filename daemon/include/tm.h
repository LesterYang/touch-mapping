#ifndef TM_H_
#define TM_H_

#include <stdint.h>
#include "qUtils.h"
#include "tmError.h"
#include "tmMapping.h"
#include "tmInput.h"

#ifndef TM_VERSION
#define TM_VERSION "?"
#endif

/************************************
 * Environment variable                                *
 *     QSI_TM_THRESHOLD : threshold             *
 *     QSI_TM_CFG            : configuration file  *
************************************/


#define __tm_list_add(head, new)                    \
({                                                  \
    new->next  = (head.next) ? head.next : NULL;    \
    head.next = new;                                \
})

#define tm_point_is_in_range(fb, x, y)              \
(!( x < (fb)->abs_begin_x   ||                      \
	x > (fb)->abs_end_x     ||                      \
	y < (fb)->abs_begin_y   ||                      \
	y > (fb)->abs_end_y   )                         \
)

typedef struct _tm_ap_info      tm_ap_info_t;
typedef struct _tm_panel_info   tm_panel_info_t;
typedef struct _tm_display      tm_display_t;

struct _tm_ap_info
{
    int                     id;
    const char*             evt_path;
    volatile int            fd;
    tm_input_type_t         touch_type;
    tm_native_size_param_t* native_size;
    volatile int            slot;
    bool                  threshold;

    list_head_t             node;
    q_mutex*                mutex;
};

struct _tm_display
{
    tm_ap_info_t*   ap;
    tm_fb_param_t   from_ap;
    tm_fb_param_t   to_pnl;
	
    list_head_t     node;
    tm_display_t*   next;
};

struct _tm_panel_info
{
    int             id;
    const char*     evt_path;
    int             fd;
    int             link_num;

    const char*     duplicate_evt_path;
    int             duplicate_fd;
    bool            duplicate;

    tm_calibrate_t* cal_param;
    tm_native_size_param_t* native_size;

    list_head_t     node;
    list_head_t     display_head;
    q_mutex*        mutex;
    q_queue*        queue;
};


tm_errno_t tm_init(void);
void     tm_deinit(void);

void             tm_fillup_fb_param(tm_fb_param_t* fb, tm_native_size_param_t* native);
tm_ap_info_t*    tm_get_ap_info(int id);
tm_panel_info_t* tm_get_panel_info(int id);
tm_ap_info_t*    tm_match_ap(int x, int y, tm_panel_info_t* panel);
tm_display_t*    tm_match_display(int x, int y, tm_panel_info_t* panel);
tm_ap_info_t*    tm_transfer(int *x, int *y, tm_panel_info_t* panel);
void             tm_duplicate_transfer(int *x, int *y, tm_panel_info_t* panel);

void tm_return_version(unsigned int len, char* from);
void tm_clear_map(unsigned int len, unsigned char *msg);
void tm_set_map(unsigned int len, unsigned char *msg);
void tm_update_ap_native_size(unsigned int len, unsigned char *msg);
void tm_switch_ap_threshold(unsigned int len, unsigned char *msg);



#endif /* TM_H_ */
