#ifndef _TMIPC_H
#define _TMIPC_H

// IPC command
#define IPC_CMD_SET_MAP  (0xa0)
#define IPC_CMD_CLR_MAP  (0xa1)

#define IPC_CMD_GET_VER  (0xd0)

#define IPC_SET_MAP_LEN  (10)
#define IPC_CLR_MAP_LEN  (1)
#define IPC_GET_VER_LEN  (0)

int  tm_open_ipc(void);
void tm_close_ipc();
void tm_recv_event(const char *from,unsigned int len,unsigned char *msg);
void tm_send_ipc(char *to, unsigned char *msg, int len);

#endif/* _TMIPC_H */
