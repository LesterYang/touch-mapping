/*
 * imIpc.c
 *
 *  Created on: Aug 1, 2014
 *      Author: lester
 */

#include <unistd.h>
#include "tm.h"
#include "tmIpc.h"

#define IPC_NAME    "QSIQT2"
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
        char *line;
        char *target;
        unsigned char *msg;
        int  len;
}tm_ipc_data_t;

tm_ipc_data_t g_client;

#endif

int tm_open_ipc()
{
#if IPC_ENABLE

    int retry;

    g_client.status = PROTOCOL_IDLE;
    g_client.recv_func = tm_recv_event;
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

void tm_close_ipc()
{
#if IPC_ENABLE
    if(g_client.server)
    {
        qsi_close_channel(g_client.server);
        g_client.server = NULL;
    }

#endif
}

void tm_recv_event(const char *from, unsigned int len, unsigned char *msg)
{
    // cmd, panel, st_x, st_y, w, h, ap, st_x, st_y, w, h
    q_dbg(Q_DBG,"recv len %d, from %s", len, from);

    switch(msg[0])
    {
        case IPC_CMD_SET_MAP:
            tm_set_map(len-1, &msg[1]);
            break;
        case IPC_CMD_CLR_MAP:
            tm_clear_map(len-1, &msg[1]);
            break;
        default:
            break;
    }
}
