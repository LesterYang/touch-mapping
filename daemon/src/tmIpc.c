/*
 *  tmIpc.c
 *  Copyright © 2014  
 *  All rights reserved.
 *  
 *       Author : Lester Yang <sab7412@yahoo.com.tw>
 *  Description : Open IPC chanel to communicate to AP
 */
 
#include <unistd.h>
#include <string.h>
#include "tm.h"
#include "tmIpc.h"

#define IPC_DEFAULT_NAME "LSTPL3"
#define IPC_ENABLE       (1)
#define IPC_DBG          (0)
#define IPC_RETRY        (3)

void tm_ipc_recv(const char *from, unsigned int len, unsigned char *msg);


int tm_ipc_open(char* name)
{
    return 0;
}

void tm_ipc_close()
{
}

void tm_ipc_recv(const char *from, unsigned int len, unsigned char *msg)
{
    q_dbg(Q_INFO,"recv len %d, from %s", len, from);

    len--;
    switch(msg[0])
    {
        case IPC_CMD_SET_MAP:
            tm_set_map(len, &msg[1]);
            break;
            
        case IPC_CMD_SET_ONE:
            if(len == IPC_SET_ONE_LEN)
            {
                tm_clear_map(IPC_CLR_MAP_LEN, &msg[1]);
                unsigned char cmd[]={0, 0, 0, 100, 100, 0, 0, 0, 100, 100};
                cmd[0] = msg[1];
                cmd[5] = msg[2];
                tm_set_map(IPC_SET_MAP_LEN, cmd);
            }
            break;
            
        case IPC_CMD_CLR_MAP:
            tm_clear_map(len, &msg[1]);
            break;
            
        case IPC_CMD_SET_APSIZE:
            tm_update_ap_native_size(len, &msg[1]);
            break;
            
        case IPC_CMD_EVT_INTR:
            tm_switch_ap_threshold(len, &msg[1]);
            break;
            
        case IPC_CMD_GET_VER:
            tm_return_version(len, (char*)from);
            break;
            
        default:
            break;
    }
}

void tm_ipc_send(char *to, unsigned char *msg, int len)
{
    g_client.status = lst_send_buffer(g_client.server, to, msg, len, IPC_NOACK);  
    
    if(g_client.status!=PROTOCOL_ACK_OK)
        q_dbg(Q_INFO, "send to %s error", to);
}
