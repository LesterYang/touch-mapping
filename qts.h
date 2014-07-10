/*
 * qts.h
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#ifndef QTS_H_
#define QTS_H_

#include <stdio.h>


#define DEFAULT_TSQSI_CONF  "/mnt/hgfs/Win_7/workspace-cpp2/libqts/qsi_ts.conf"
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


// struct tsqsi_dev :
// horizontal is 0 if event value increments from left to right
// vertical is 0 if event value increments from top to bottom
// swap is 0 if x is horizontal
struct tsqsi_dev
{
    short min_x;
    short max_x;
    short min_y;
    short max_y;
    char swap;
    char horizontal;
    char vertical;
    char used;
    unsigned char name_len;
    char* name;
};

struct tsqsi
{
    int fd;
    FILE *fr;
    struct tsqsi_dev panel[MAX_DEV_NUM];
    struct tsqsi_dev fb[MAX_DEV_NUM];
};

const char* tsqsi_errorno_str(qerrno no);
qerrno      tsqsi_create(struct tsqsi** p_ts);
void        tsqsi_destroy(struct tsqsi* ts);
short       tsqsi_calculate_permille(short val, short min, short max, char reverse);
short       tsqsi_calculate_output(short permille, short min, short max);
qerrno      __tsqsi_transfer(short* x, short* y, struct tsqsi_dev* src, struct tsqsi_dev* dest);
qerrno      tsqsi_transfer(short* x, short* y, struct tsqsi* ts, unsigned char panel, unsigned char fb);

#endif /* QTS_H_ */
