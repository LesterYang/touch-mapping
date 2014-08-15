/*
 * qInput.h
 *
 *  Created on: Aug 1, 2014
 *      Author: lester
 */

#ifndef TMINPUT_H_
#define TMINPUT_H_

//#include "tm.h"
#include "tmError.h"
#include "tmMapping.h"

#define LONG_BITS (sizeof(long) << 3)
#define NUM_LONGS(bits) (((bits) + LONG_BITS - 1) / LONG_BITS)

static inline int testBit(long bit, const long *array)
{
    return (array[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

typedef enum _tm_input_status tm_input_status_t;
typedef enum _tm_input_type tm_input_type_t;

typedef struct input_event tm_input_event_t;
typedef struct timeval tm_input_timeval_t;

enum _tm_input_status{
    //TM_INPUT_STATUS_START,
   // TM_INPUT_STATUS_PASS,
   // TM_INPUT_STATUS_COLLECT,
  //  TM_INPUT_STATUS_MT_COLLECT,
   // TM_INPUT_STATUS_MT_END,
   // TM_INPUT_STATUS_END,

    TM_INPUT_STATUS_TOUCH,
    TM_INPUT_STATUS_PRESS,
    TM_INPUT_STATUS_DRAG,
    TM_INPUT_STATUS_RELEASE,
    TM_INPUT_STATUS_IDLE,

    TM_INPUT_STATUS_MT_TOUCH,
    TM_INPUT_STATUS_MT_RELEASE,
    TM_INPUT_STATUS_MT_IDLE,

    TM_INPUT_STATUS_MT_B_TRANS,
    TM_INPUT_STATUS_MT_A_SYNC,

    TM_INPUT_STATUS_NONE = -1
};

enum _tm_input_type{
    TM_INPUT_TYPE_SINGLE,
    TM_INPUT_TYPE_MT_A,
    TM_INPUT_TYPE_MT_B,

    TM_INPUT_TYPE_NONE = -1
};


#define set_abs_status(status)                  \
do{                                             \
    if(status == TM_INPUT_STATUS_COLLECT)       \
        status = TM_INPUT_STATUS_TRANS;         \
    else                                        \
        status = TM_INPUT_STATUS_COLLECT;       \
}while(0)


static inline void tm_input_get_time(tm_input_timeval_t *time)
{
    gettimeofday(time, NULL);
}

tm_errno_t  tm_input_init(list_head_t* ap_head, list_head_t* panel_head);
void tm_input_deinit();

#endif /* TMINPUT_H_ */
