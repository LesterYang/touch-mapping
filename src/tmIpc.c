/*
 *  tmIpc.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : Open IPC chanel to communicate to AP
 */
 
#include <unistd.h>
#include "tm.h"
#include "tmIpc.h"

#define IPC_NAME    "QSIPL3"
#define IPC_ENABLE  (1)
#define IPC_DBG     (0)
#define IPC_RETRY   (3)

#if IPC_ENABLE

#include <qsi_ipc_client_lib.h>

typedef struct _tm_ipc_data{
        QSI_Channel *server;
        QSI_RECV_EVENT recv_func;
        QSI_PROTOCOL_ST status;
        char *name;
        char debug;
        char *target;
        unsigned char *msg;
        int  len;
}tm_ipc_data_t;

tm_ipc_data_t g_client;

#endif

void tm_ipc_recv(const char *from, unsigned int len, unsigned char *msg);


int tm_ipc_open()
{
#if IPC_ENABLE

    int retry;

    g_client.status = PROTOCOL_IDLE;
    g_client.recv_func = tm_ipc_recv;
    g_client.name = IPC_NAME;

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

    switch(msg[0])
    {
        case IPC_CMD_SET_MAP:
            // cmd, panel, st_x, st_y, w, h, ap, st_x, st_y, w, h
            tm_set_map(len-1, &msg[1]);
            break;
        case IPC_CMD_CLR_MAP:
            tm_clear_map(len-1, &msg[1]);
            break;
        case IPC_CMD_GET_VER:
            tm_return_version(len-1, (char*)from);
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
