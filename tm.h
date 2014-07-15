/*
 * tm.h
 *
 *  Created on: Jul 11, 2014
 *      Author: root
 */

#ifndef TM_H_
#define TM_H_

#include <stdint.h>
#include "tmMap.h"
#include "qUtils.h"

#define Queue_HdrLen    (2)
#define Queue_Hdr0      (0xff)
#define Queue_Hdr1      (0x55)

typedef enum tm_main_status{
	eTmStatusNone,
    eTmStatusInputeInit,
    eTmStatusSocketInit,
    eTmStatusLoop,
    eTmStatusDeinit,
    eTmStatusError,
    eTmStatusExit
}tm_main_status;

struct sTmData
{
    int fd;
    FILE *fr;
    q_mutex* mutex;
    q_queue* queue;
    tm_main_status status;
    struct sTmDevParam panel[eTmDevNum];
    struct sTmDevParam fb[eTmDevNum];
};

void    tm_switch_main_status(tm_main_status status);
tm_main_status tm_get_main_status();
qerrno  tm_init(void);
void    tm_deinit(void);
void    tm_shutdown(int signum);
void    tm_set_dev_param(struct sEventDev* evDev, struct sTmDevParam* dataDev);
void    tm_set_direction(tm_dev source, tm_ap target_ap);
void    tm_save_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op);
void    tm_send_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op);
void    tm_parse_event(void);

#endif /* TM_H_ */
