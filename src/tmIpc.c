/*
 *  tmIpc.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : Open IPC chanel to communicate to AP
 */
 
#include <unistd.h>
#include <string.h>
#include "tm.h"
#include "tmIpc.h"

#define IPC_DEFAULT_NAME "QSIPL3"
#define IPC_ENABLE       (1)
#define IPC_DBG          (0)
#define IPC_RETRY        (3)

#if IPC_ENABLE

#include <qsi_ipc_client_lib.h>

typedef struct _tm_ipc_data{
        QSI_Channel *server;
        QSI_RECV_EVENT recv_func;
        QSI_PROTOCOL_ST status;
        char name[IPC_MAX_NAME];
        char debug;
        char *target;
        unsigned char *msg;
        int  len;
}tm_ipc_data_t;

static tm_ipc_data_t g_client;

#endif

void tm_ipc_recv(const char *from, unsigned int len, unsigned char *msg);


int tm_ipc_open(char* name)
{
#if IPC_ENABLE

    int retry, len;

    g_client.status = PROTOCOL_IDLE;
    g_client.recv_func = tm_ipc_recv;
    if (name && *name && (len=strlen(name))<IPC_MAX_NAME) 
    {
        memcpy(g_client.name, name, len);
    }
    else 
    {
        memcpy(g_client.name, IPC_DEFAULT_NAME, strlen(IPC_DEFAULT_NAME));
    }

    for(retry=0;;retry++)
    {
        g_client.server = qsi_open_channel(g_client.name, 0, IPC_DBG);

        if(g_client.server != NULL)
            break;

        usleep(100000);

        if(retry == IPC_RETRY)
        {
            q_dbg(Q_INFO,"open \"%s\" channel status timeout",g_client.name);
            return 1;
        }
    }

    q_dbg(Q_INFO,"open IPC client : \"%s\"",g_client.name);
    // set receiving event callback function
    qsi_set_event(g_client.server, g_client.recv_func);

#endif
    return 0;
}

void tm_ipc_close()
{
#if IPC_ENABLE
    if(g_client.server)
    {
        qsi_close_channel(g_client.server);
        g_client.server = NULL;
    }

#endif
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
    g_client.status = qsi_send_buffer(g_client.server, to, msg, len, IPC_NOACK);  
    
    if(g_client.status!=PROTOCOL_ACK_OK)
        q_dbg(Q_INFO, "send to %s error", to);
}
