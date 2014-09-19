#ifndef _TMIPC_H
#define _TMIPC_H

// IPC command
#define IPC_CMD_SET_MAP  (0xa0)
#define IPC_CMD_CLR_MAP  (0xa1)

#define IPC_SET_MAP_LEN  (10)
#define IPC_CLR_MAP_LEN  (1)

int  tm_open_ipc(void);
void tm_close_ipc();
void tm_recv_event(const char *from,unsigned int len,unsigned char *msg);

#endif/* _TMIPC_H */
