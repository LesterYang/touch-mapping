#ifndef _TM_TEST_H
#define _TM_TEST_H

#ifndef TM_TEST_VERSION
#define TM_TEST_VERSION "?"
#endif

#define TEST_DEBUG  (1)

#define default_evt "/dev/input/event0"
#define default_fb  "/dev/fb0"
#define default_pan "/sys/class/graphics/fb0/pan"

#define TEST_CFG_FILE    "/home/qsi_tm/qsi_tm.conf"
#define TEST_IPC_NAME    "QSIPL2"
#define TEST_IPC_TARGET  "QSIPL3"

#define EVT_LEN     (1+sizeof(default_evt))
#define FB_LEN      (1+sizeof(default_fb))
#define PAN_LEN     (1+sizeof(default_pan))

#define EVT_NUM_POS (16)
#define FB_NUM_POS  (7)
#define PAN_NUM_POS (22)

#define PNL_NUM     (3)

#define PNL0_FB_NUM      (0)
#define PNL1_FB_NUM      (4)
#define PNL2_FB_NUM      (9)
#define PNL0_DEFAULT_AP  (0)
#define PNL1_DEFAULT_AP  (4)
#define PNL2_DEFAULT_AP  (6)
#define PNL0_DEFAULT_EVT (0)
#define PNL1_DEFAULT_EVT (4)
#define PNL2_DEFAULT_EVT (6)
#define PNL0_ORG_EVT     (20)
#define PNL1_ORG_EVT     (21)
#define PNL2_ORG_EVT     (22)

#ifdef __GNUC__
#define Q_MAX(a,b)                              \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a > _b ? _a : _b;                  \
        })
#define Q_MIN(a,b)                              \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a < _b ? _a : _b;                  \
        })
#else
#define Q_MAX(a, b) 			((a) > (b) ? (a) : (b))
#define Q_MIN(a, b) 			((a) < (b) ? (a) : (b))
#endif

#define Q_ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

enum test_mode
{
    MONO_AP = 0,
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

enum pnl_num
{
    MONO_PNL = 0,
    DE_PNL,
    TRI_PNL,
    TETRA_PNL,
    PENTA_PNL,

    MAX_PNL_NUM
};

#define MAX_STR_LEN (32)
#define NR_BUTTONS  (MAX_AP_NUM + 3)
#define STR_NUM     (MAX_AP_NUM + 1)


typedef struct _fb_data fb_data_t;
typedef struct _evt_data evt_data_t;

struct _fb_data{
    int   fb_id;
    int   pnl_id;
    int   ap_id;
    char  dev[FB_LEN];
    char  pan[PAN_LEN];
    char  str[STR_NUM][MAX_STR_LEN];
};

struct _evt_data{
    int   num;
    int   act;
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
int ts_cal(fb_data_t* fb, char* evt_path);
int replace_config(fb_data_t* fb, evt_data_t* evt);
int refresh_tm_test(fb_data_t* fb);
void set_button_num(int num);
int open_framebuffer(fb_data_t* fb);
void update_calibrate(void);
#endif
