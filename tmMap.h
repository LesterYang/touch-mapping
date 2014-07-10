/*
 * tmMap.h
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#ifndef QTS_H_
#define QTS_H_

#include <stdio.h>


#define QSI_TM_CONF         "/mnt/hgfs/Win_7/workspace-cpp2/touch-mapping/qsi_tm.conf"
#define BUF_SIZE            (256)
#define MAX_DEV_NUM         (5)
#define MULTIPLE            (4096)

typedef enum qerrno{
    eENoErr     =  0,       // No error
    eENoDev     = -1,       // No such device
    eEDevP      = -2,       // Parameter of device error
    eEDevU      = -3,       // Device isn't used
    eEDevN      = -4,       // Bad device number
    eEAlloc     = -5,       // Allocate error
    eEOpen      = -6,       // Open file error
    eEPoint     = -7        // Points are out of rage
}qerrno;


// struct sTmData_dev :
// horizontal is 0 if event value increments from left to right
// vertical is 0 if event value increments from top to bottom
// swap is 0 if x is horizontal
struct sTmData_dev
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

struct sTmData
{
    int fd;
    FILE *fr;
    q_mutex* mutex;
    struct sTmData_dev panel[MAX_DEV_NUM];
    struct sTmData_dev fb[MAX_DEV_NUM];
};

const char* tm_errorno_str(qerrno no);
qerrno      tm_create(struct sTmData** p_tm);
void        tm_destroy(struct sTmData* tm);
qerrno      tm_update_conf(struct sTmData* tm);
short       tm_calculate_permille(short val, short min, short max, char reverse);
short       tm_calculate_output(short permille, short min, short max);
qerrno      __tm_transfer(short* x, short* y, struct sTmData_dev* src, struct sTmData_dev* dest);
qerrno      tm_transfer(short* x, short* y, struct sTmData* tm, unsigned char panel, unsigned char fb);

#endif /* QTS_H_ */
