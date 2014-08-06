/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#include <signal.h>
#include <unistd.h>
#include "tm.h"

#define IPC_ENABLE  (0)
#define IPC_NAME    "QSIDBG"
#define IPC_DBG     (0)
#define IPC_RETRY   (3)

struct tm_status_info{
    tm_status_t status;
    const char* str;
};

#if IPC_ENABLE

#include "/Space/ltib2/ltib/rootfs/usr/include/qsi_ipc_client_lib.h"

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

struct tm_status_info status_info[] = {
    {TM_STATUS_NONE,        "None"},
    {TM_STATUS_INIT,        "Init"},
    {TM_STATUS_IPC_INIT,    "IPCInit"},
    {TM_STATUS_RUNNING,     "Running"},
    {TM_STATUS_DEINIT,      "Deinit"},
    {TM_STATUS_ERROR,       "Error"},
    {TM_STATUS_EXIT,        "Exit"},
};

tm_status_t g_status = TM_STATUS_NONE;

void tm_test(void);
void tm_init_ipc(void);
void tm_deinit_ipc();
void tm_recv_event(const char *from,unsigned int len,unsigned char *msg);

void tm_shutdown(int signum)
{
    tm_deinit();
    tm_deinit_ipc();
    exit(signum);
}

void tm_switch_main_status(tm_status_t status)
{
     q_dbg("status : %10s -> %s", q_strnull(status_info[g_status].str), q_strnull(status_info[status].str));
     g_status = status;
}

int main(int argc, char* argv[])
{

    // do fork ....

    while(g_status != TM_STATUS_EXIT)
    {
        switch(g_status)
        {
            case TM_STATUS_NONE:
                tm_switch_main_status(TM_STATUS_INIT);
                break;
            case TM_STATUS_INIT:
                if(tm_init() == TM_ERRNO_SUCCESS)
                {
                    signal(SIGINT, tm_shutdown);
                    tm_bind_status(&g_status);
                    tm_switch_main_status(TM_STATUS_IPC_INIT);
                }
                else
                    sleep(1);
                break;
            case TM_STATUS_IPC_INIT:
                // do IPC ...
            	tm_init_ipc();
                tm_switch_main_status(TM_STATUS_RUNNING);
                break;
            case TM_STATUS_RUNNING:
#if 1 //test
                tm_test();
                tm_switch_main_status(TM_STATUS_DEINIT);
#else
                sleep(1);
#endif
                break;
            case TM_STATUS_DEINIT:
                tm_deinit();
                tm_switch_main_status(TM_STATUS_EXIT);
                break;
			case TM_STATUS_REINIT:
				tm_deinit();
				tm_switch_main_status(TM_STATUS_INIT);
				break;
            case TM_STATUS_ERROR:
                tm_switch_main_status(TM_STATUS_DEINIT);
                break;
            case TM_STATUS_EXIT:
                break;
            default:
                tm_switch_main_status(TM_STATUS_ERROR);
                break;
        }
    }
    return 0;
}

void tm_init_ipc()
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
            printf("open \"%s\" channel status timeout\n",g_client.name);
            tm_switch_main_status(TM_STATUS_ERROR);
            return;
        }
    }

    qsi_set_event(g_client.server, g_client.recv_func);     // set receiving event callback function

#endif
}

void tm_deinit_ipc()
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
    q_dbg("recv len %d, from %s", len, from);

    switch(msg[0])
    {
        case IPC_CMD_SET_MAP:
            tm_set_map(len-1, &msg[1], q_false);
            break;
        case IPC_CMD_ADD_MAP:
            tm_set_map(len-1, &msg[1], q_true);
            break;
        default:
            break;
    }
}
