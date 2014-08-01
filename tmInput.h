/*
 * qInput.h
 *
 *  Created on: Jul 9, 2014
 *      Author: lester
 */

#ifndef TMINPUT_H_
#define TMINPUT_H_

#include "tm.h"
#include "tmError.h"
#include "tmMapping.h"

typedef enum _tm_input_status tm_input_status_t;

typedef struct input_event tm_input_event_t;

enum _tm_input_status{
    TM_INPUT_STATUS_START,
    TM_INPUT_STATUS_PASS,
    TM_INPUT_STATUS_COLLECT,
    TM_INPUT_STATUS_MT_COLLECT,
    TM_INPUT_STATUS_TRANS,
    TM_INPUT_STATUS_MT_END,
    TM_INPUT_STATUS_END,

    TM_INPUT_STATUS_NONE = -1
};

#define set_abs_status(status)                  \
do{                                             \
    if(status == TM_INPUT_STATUS_START)         \
        status = TM_INPUT_STATUS_COLLECT;       \
    else if(status == TM_INPUT_STATUS_COLLECT)  \
        status = TM_INPUT_STATUS_TRANS;         \
}while(0)


tm_errno_t  tm_input_init(tm_panel_info_t* panel, tm_ap_info_t* ap);
void tm_input_deinit();

#endif /* TMINPUT_H_ */
