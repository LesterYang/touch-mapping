/*
 * _tm.h
 *
 *  Created on: Jul 18, 2014
 *      Author: root
 */

#ifndef TM_H_
#define TM_H_

#include <stdint.h>
#include "_tmMap.h"
#include "qUtils.h"

#define TM_HDR_LEN    (2)
#define TM_HDR_0      (0xff)
#define TM_HDR_1      (0x55)

typedef enum _tm_status     tm_status_t;
typedef enum _tm_errno      tm_errno_t;

typedef struct _tm_handler  tm_handler_t;

enum _tm_errno{
    TM_ERRNO_SUCCESS    =  0,       // No error
    TM_ERRNO_NO_DEV     = -1,       // No such device or device doesn't initialize
    TM_ERRNO_DEV_PARAM  = -2,       // Parameter of device error
 // TM_ERRNO_           = -3,       //
    TM_ERRNO_DEV_NUM    = -4,       // Bad device number
    TM_ERRNO_ALLOC      = -5,       // Allocate error
    TM_ERRNO_OPEN       = -6,       // Open file error
    TM_ERRNO_POINT      = -7,       // Points are out of rage
    TM_ERRNO_PARAM      = -8,       // Function parameter error
    TM_ERRNO_SWAP       = -9        // Need to swap x,y
};

enum _tm_status{
    TM_STATUS_NONE,
    TM_STATUS_INPUT_INIT,
    TM_STATUS_SOCKET_INIT,
    TM_STATUS_RUNNING,
    TM_STATUS_DEINIT,
    TM_STATUS_ERROR,
    TM_STATUS_EXIT
};

struct _tm_handler
{
    int fd;
    FILE *fr;
    q_mutex* mutex;
    q_queue* queue;
    tm_status_t status;
    tm_fb_param_t fb[TM_PANEL_NUM];
};


#endif /* TM_H_ */
