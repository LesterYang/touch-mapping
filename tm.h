/*
 * tm.h
 *
 *  Created on: Jul 11, 2014
 *      Author: root
 */

#ifndef TM_H_
#define TM_H_



qerrno  tm_init(void);
void    tm_deinit(void);
void    tm_shutdown(int signum);
void    tm_set_dev_param(struct sEventDev* evDev, struct sTmDataDev* dataDev);

#endif /* TM_H_ */
