/*
 * tm.h
 *
 *  Created on: Jul 11, 2014
 *      Author: root
 */

#ifndef TM_H_
#define TM_H_

#include "tmMap.h"

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
}tm_main_status;

struct sTmData
{
    int fd;
    FILE *fr;
    q_mutex* mutex;
    q_queue* queue;
    struct sTmDataDev panel[eTmDevNum];
    struct sTmDataDev fb[eTmDevNum];
};

qerrno  tm_init(void);
void    tm_deinit(void);
void    tm_shutdown(int signum);
void    tm_set_dev_param(struct sEventDev* evDev, struct sTmDataDev* dataDev);
void    tm_save_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op);
void    tm_send_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op);
void    tm_parse_event(void);

#endif /* TM_H_ */
