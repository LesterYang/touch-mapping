/*
 * tm_error.h
 *
 *  Created on: Jul 21, 2014
 *      Author: root
 */

#ifndef TMERROR_H_
#define TMERROR_H_

typedef enum _tm_errno tm_errno_t;

enum _tm_errno{
    TM_ERRNO_SUCCESS    =  0,       // No error
    TM_ERRNO_NO_DEV     = -1,       // No such device or device doesn't initialize
    TM_ERRNO_DEV_PARAM  = -2,       // Parameter of device error
    TM_ERRNO_DEV_NUM    = -4,       // Bad device number
    TM_ERRNO_ALLOC      = -5,       // Allocate error
    TM_ERRNO_OPEN       = -6,       // Open file error
    TM_ERRNO_POINT      = -7,       // Points are out of rage
    TM_ERRNO_PARAM      = -8,       // Function parameter error
    TM_ERRNO_SWAP       = -9,       // Need to swap x,y
    TM_ERRNO_NO_CONF    = -10,      // No configuration
    TM_ERRNO_NO_FD     = -11        // Event doesn't opened
};

static inline const char* tm_err_str(tm_errno_t no)
{
    switch(no)
    {
        case TM_ERRNO_SUCCESS:      return " No error";
        case TM_ERRNO_NO_DEV:       return " No such device";
        case TM_ERRNO_DEV_PARAM:    return " Parameter of device error";
        case TM_ERRNO_DEV_NUM:      return " Bad device number";
        case TM_ERRNO_ALLOC:        return " Allocate error";
        case TM_ERRNO_OPEN:         return " Open file error";
        case TM_ERRNO_POINT:        return " Points are out of rage";
        case TM_ERRNO_PARAM:        return " Function parameter error";
        case TM_ERRNO_SWAP:         return " Need to swap xy";
        case TM_ERRNO_NO_CONF:      return " No configuration";
        case TM_ERRNO_NO_FD:     	return "Event doesn't opened";
        default:            break;
    }
    return "unknown";
}

#endif /* TMERROR_H_ */
