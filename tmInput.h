/*
 * qInput.h
 *
 *  Created on: Jul 9, 2014
 *      Author: lester
 */

#ifndef TMINPUT_H_
#define TMINPUT_H_

#include "tmMapping.h"

typedef enum _tm_input_status tm_input_status_t;
typedef struct _tm_input_handler tm_input_handler_t;


typedef struct input_event tm_input_event_t;


enum _tm_input_status{
    TM_INPUT_STATUS_START,
    TM_INPUT_STATUS_COLLECT,
    TM_INPUT_STATUS_TRANS,
    TM_INPUT_STATUS_MT_END,
    TM_INPUT_STATUS_END
};

struct _tm_input_handler {
	tm_panel_info_t* panel;
    fd_set skfds;
    fd_set evfds;
    int maxfd;
    q_thread  *thread;
    tm_input_status_t status;
};



int  tm_inputInit(struct sEventDev* evDev);
void tm_inputDeinit();


int  tm_input_init(tm_panel_info_t* panel, tm_event_info_t* event);
void tm_input_deinit();
void tm_send_event(tm_panel_info_t* tm_input); // Global array in tmInput.c, tm_input_handler_t tm_input[TM_PANEL_NUM]
void tm_input_parse_event(tm_input_handler_t* tm_input);


int  tm_input_init_events(tm_panel_info_t* tm_input);
void tm_input_close_events(tm_panel_info_t* tm_input);
void tm_input_clean_stdin(tm_panel_info_t* tm_input);
int  tm_input_add_fd (tm_panel_info_t* tm_input, fd_set * fdsp);
static void tm_input_thread_func(void *data);



#endif /* TMINPUT_H_ */
