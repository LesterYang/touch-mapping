#ifndef _TM_TEST_H
#define _TM_TEST_H

#define default_evt "/dev/input/event0"
#define default_fb  "/dev/fb0"
#define default_pan "/sys/class/graphics/fb0/pan"

#define EVT_LEN     (1+sizeof(default_evt))
#define FB_LEN      (1+sizeof(default_fb))
#define PAN_LEN     (1+sizeof(default_pan))

#define EVT_NUM_POS (16)
#define FB_NUM_POS  (7)
#define PAN_NUM_POS (22)

#define PNL_NUM     (3)

#define PNL0_FB_NUM (0)
#define PNL1_FB_NUM (4)
#define PNL2_FB_NUM (9)
#define PNL0_DEFAULT_AP (0)
#define PNL1_DEFAULT_AP (4)
#define PNL2_DEFAULT_AP (6)

#define MAX_STR_LEN (32)
#define NR_BUTTONS  (9)
#define STR_NUM     (NR_BUTTONS - 2)

enum test_mode
{
    MONO_AP,
    DE_AP,
    TRI_AP,
    TETRA_AP,
    PENTA_AP,
    HEXA_AP ,
    HEPTA_AP,
    OCTA_AP,
    NONA_AP,
    DECA_AP,
    
    MAX_AP_NUM
};


typedef struct _fb_data fb_data_t;
typedef struct _evt_data evt_data_t;

struct _fb_data{
    int   fb_num;
    int   pnl_id;
    int   ap_id;
    int   pnl_arg;
    int   ap_arg[MAX_AP_NUM];
    fb_data_t* m;
    char  dev[FB_LEN];
    char  pan[PAN_LEN];
    char  str[STR_NUM][MAX_STR_LEN];
};

struct _evt_data{
    int   num;
    char  dev[EVT_LEN];
    char  calibrate;
};


#define dbg_log(expr, ...)                                               \
do {                                                                     \
    fprintf(stderr, "tm-test : failed at %s:%u, ",  __FILE__, __LINE__); \
    fprintf(stderr, expr,  ##__VA_ARGS__);                               \
    fprintf(stderr, "\n");                                               \
} while (0)


int ts_test(fb_data_t* fb, evt_data_t* evt);
int open_framebuffer(fb_data_t* fb);
int open_slave_fb(fb_data_t* fb);
void close_slave_fb(fb_data_t* fb);

#endif
