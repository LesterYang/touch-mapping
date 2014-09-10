/*
 *  tmIpc.h
 *
 *  Created on: Sep 10, 2014
 *      Author: lester
 */

#ifndef _TMIPC_H
#define _TMIPC_H

int  tm_open_ipc(void);
void tm_close_ipc();
void tm_recv_event(const char *from,unsigned int len,unsigned char *msg);

#endif/* _TMIPC_H */
