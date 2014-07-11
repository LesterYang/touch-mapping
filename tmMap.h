/*
 * tmMap.h
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#ifndef QTS_H_
#define QTS_H_

#include <stdio.h>
#include <linux/input.h>

#define QSI_TM_CONF         "/mnt/hgfs/Win_7/workspace-cpp2/touch-mapping/qsi_tm.conf"
#define BUF_SIZE            (256)
#define MULTIPLE            (4096)
#define MAX_QUEUE           (256)

typedef struct input_event sInputEv;

typedef enum tm_ap{
    eTmApQSI,
    eTmApQSILeft,
    eTmApQSIRight,
    eTmApNavi,
    eTmApMonitor,

    eTmApMax,
    eTmApNum = eTmApMax,

    eTmApNone = -1
}tm_ap;

typedef enum tm_dev{
    eTmDevFront,
    eTmDevLeft,
    eTmDevRight,

    eTmDevMax,
    eTmDevNum = eTmDevMax,

    eTmDevNone = -1
}tm_dev;


typedef enum qerrno{
    eENoErr     =  0,       // No error
    eENoDev     = -1,       // No such device
    eEDevP      = -2,       // Parameter of device error
    eEDevU      = -3,       // Device isn't used
    eEDevN      = -4,       // Bad device number
    eEAlloc     = -5,       // Allocate error
    eEOpen      = -6,       // Open file error
    eEPoint     = -7,       // Points are out of rage
    eEParam     = -8,       // Function parameter error
    eESwap      = -9        // Need to swap x,y
}qerrno;

typedef enum tm_event_code{
    eTmEventNone,
    eTmEventX,
    eTmEventY,
    eTmEventMtX,
    eTmEventMtY
}tm_event_code;

typedef enum tm_op_code{
    eTmOpCodeNone,
    eTmOpCodeTrans,
    eTmOpCodeStart,
    eTmOpCodeEnd,
    eTmOpCodeMtStart,
    eTmOpCodeMtEnd
}tm_op_code;


struct sInputEvDev
{
  const char *evDevPath;
  int fd;
  tm_dev displayDev;
  tm_ap  displayAp;
};

// struct sTmDataDev :
// horizontal is 0 if event value increments from left to right
// vertical is 0 if event value increments from top to bottom
// swap is 0 if x is horizontal
struct sTmDataDev
{
    short min_x;
    short max_x;
    short min_y;
    short max_y;
    char swap;
    char horizontal;
    char vertical;
    char used;
};

struct sEventDev
{
    const char*         evDevPath;
    int                 fd;
    tm_dev              dev;
    tm_ap               ap;
    struct sTmDataDev*  paramDev;

      union{
        struct sEventDev* sourceDev;
        struct sEventDev* targetDev;
      };
};

struct sTmData
{
    int fd;
    FILE *fr;
    q_mutex* mutex;
    q_queue* queue;
    struct sTmDataDev panel[eTmDevNum];
    struct sTmDataDev fb[eTmDevNum];
};

const char* tm_errorno_str(qerrno no);
qerrno      tm_create(struct sTmData** p_tm);
void        tm_destroy(struct sTmData* tm);
qerrno      tm_update_conf(struct sTmData* tm);
short       tm_calculate_permille(short val, short min, short max, char reverse);
short       tm_calculate_output(short permille, short min, short max);
qerrno      __tm_transfer(short* x, short* y, struct sTmDataDev* src, struct sTmDataDev* dest);
qerrno      tm_transfer(short* x, short* y, struct sTmData* tm, unsigned char panel, unsigned char fb);

qerrno      __tm_transfer_value(short* val, tm_event_code code, struct sTmDataDev* src, struct sTmDataDev* dest);
qerrno      tm_transfer_ev(sInputEv *ev, struct sTmData* tm, unsigned char panel, unsigned char fb);

void        tm_save_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op);
void        tm_send_event(sInputEv *ev, tm_dev from, tm_op_code op);

qerrno      tm_set_direction(struct sTmData* tm, tm_dev source, tm_ap target_ap);

#endif /* QTS_H_ */
