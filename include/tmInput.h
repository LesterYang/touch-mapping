#ifndef TMINPUT_H_
#define TMINPUT_H_

//#include "tm.h"
#include "tmError.h"
#include "tmMapping.h"
#include <time.h>


#define LONG_BITS (sizeof(long) << 3)
#define NUM_LONGS(bits) (((bits) + LONG_BITS - 1) / LONG_BITS)
#define SLOT_NUM (5)
#define DEFAULT_THRESHOLD "0"
#define THRESHOLD_UNIT          (100000)     //100ms
#define THRESHOLD_CLOCK_UNIT    (100000000)  //100ms


typedef enum _tm_input_status tm_input_status_t;
typedef enum _tm_input_type tm_input_type_t;

typedef struct input_event tm_input_event_t;
typedef struct timeval tm_input_timeval_t;
typedef struct timespec tm_input_timespec_t;


enum _tm_input_status
{
    TM_INPUT_STATUS_IDLE=0,
    TM_INPUT_STATUS_TOUCH,
    TM_INPUT_STATUS_PRESS,
    TM_INPUT_STATUS_DRAG,
    TM_INPUT_STATUS_RELEASE,

    TM_INPUT_STATUS_MT_IDLE=0, 
    TM_INPUT_STATUS_MT_PRESS,
    TM_INPUT_STATUS_MT_RELEASE,
    TM_INPUT_STATUS_MT_DRAG,

    TM_INPUT_STATUS_NONE = -1
};

enum _tm_input_type
{
    TM_INPUT_TYPE_SINGLE, 
    TM_INPUT_TYPE_MT_A,
    TM_INPUT_TYPE_MT_B,

    TM_INPUT_TYPE_NONE = -1
};


static inline int testBit(long bit, const long *array)
{
    return (array[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

static inline void tm_input_get_time(tm_input_timeval_t *time)
{
    gettimeofday(time, NULL);
}

static inline void tm_input_get_clock_time(tm_input_timespec_t *clock)
{
    clock_gettime(CLOCK_REALTIME, clock);
}


static inline int tm_input_elapsed_time(tm_input_timeval_t *now, tm_input_timeval_t *old)
{
    return ((now->tv_sec - old->tv_sec) * 1000000 + (now->tv_usec - old->tv_usec))/THRESHOLD_UNIT;
}

static inline int tm_input_elapsed_clock_time(tm_input_timespec_t *now, tm_input_timespec_t *old)
{
    return ((now->tv_sec - old->tv_sec) * 10) + ((now->tv_nsec - old->tv_nsec)/THRESHOLD_CLOCK_UNIT);
}


static inline void tm_input_add_time(tm_input_timeval_t *time, int ms)
{
    time->tv_usec += ms;

    if((time->tv_usec / 1000000))
    {
        time->tv_sec++;
    }
    time->tv_usec %= 1000000;
}

tm_errno_t tm_input_init(list_head_t* ap_head, list_head_t* pnl_head);
void       tm_input_deinit(void);


#endif /* TMINPUT_H_ */
