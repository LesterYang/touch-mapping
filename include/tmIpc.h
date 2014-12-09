#ifndef _TMIPC_H
#define _TMIPC_H

// IPC command
#define IPC_CMD_SET_MAP  (0xa0)
#define IPC_CMD_SET_ONE  (0xa1)
#define IPC_CMD_CLR_MAP  (0xa2)

#define IPC_CMD_SET_APSIZE (0xb0)

#define IPC_CMD_GET_VER  (0xd0)


// command length
#define IPC_SET_MAP_LEN  (10)
#define IPC_SET_ONE_LEN  (2)
#define IPC_CLR_MAP_LEN  (1)

#define IPC_CMD_SET_APSIZE_LEN (5) 

#define IPC_GET_VER_LEN  (0)



#define IPC_MAX_NAME     (8)

int  tm_ipc_open(char* name);
void tm_ipc_close();
void tm_ipc_send(char *to, unsigned char *msg, int len);

#endif/* _TMIPC_H */
