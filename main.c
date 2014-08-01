/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#include <signal.h>
#include <unistd.h>
#include "tm.h"

struct tm_status_info{
    tm_status_t status;
    const char* str;
};

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

void tm_test();

void tm_shutdown(int signum)
{
    tm_deinit();
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

