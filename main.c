/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#include <signal.h>
#include <unistd.h>
#include "tm.h"

#if 1

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
                    signal(SIGKILL, tm_shutdown);
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
                //sleep(1);
#if 1 // test
                {
                    int x=200,y=300;
                    tm_transfer(&x, &y, NULL);
                }
                //tm_mapping_test();
                tm_switch_main_status(TM_STATUS_DEINIT);
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

#else
void tm_check_per(short x, short y, struct sTmDevParam* dev)
{
    int perx = (((int)x - dev->min_x) * MULTIPLE) / (dev->max_x - dev->min_x);
    int pery = (((int)y - dev->min_y) * MULTIPLE) / (dev->max_y - dev->min_y);
    printf("x %2d%% y %2d%%\n",(perx*100)/MULTIPLE, (pery*100)/MULTIPLE);
}






int main(int argc, char* argv[])
{

// fork ....

	int count = 0; // for test

	while(tm_get_main_status() != eTmStatusExit)
	{
		switch(tm_get_main_status())
		{
			case eTmStatusInputeInit:
			    if(tm_init() != eENoErr)
			        return 0;
			    tm_switch_main_status(eTmStatusSocketInit);
			    break;

			case eTmStatusSocketInit:
				tm_switch_main_status(eTmStatusLoop);
				break;

			case eTmStatusLoop:

#if 1 // for test
			    while((++count) < 100)
			    {
			        tm_parse_event();
			        usleep(100000);
			    }
			    //sleep(5);
			    tm_switch_main_status(eTmStatusDeinit);
#endif
				break;

			case eTmStatusDeinit:
				//  close socket ..
				tm_deinit();
				tm_switch_main_status(eTmStatusExit);
				break;

			case eTmStatusError:
				tm_switch_main_status(eTmStatusDeinit);
				break;

			case eTmStatusExit:
			    break;
			default:
				tm_switch_main_status(eTmStatusError);
				break;
		}
	}






// socket init ...
// while loop...

#if 0 // test points
    short x,y;
    unsigned char p,f;
    qerrno err_no;

    if (argc == 5)
    {
        x = atoi(argv[1]);
        y = atoi(argv[2]);
        p = atoi(argv[3]);
        f = atoi(argv[4]);
    }
    else
    {
        x = -10;
        y = 500;
        p = 2;
        f = 0;
    }

    tm_set_direction(tm, eTmDevRight, eTmDevFront);

    tm_check_per(x,y,&tm->panel[p]);
    q_dbg("panel%d to fb%d : (%d,%d) -> ",p,f,x,y);
    if((err_no = tm_transfer(&x, &y, tm, p, f))!= eENoErr)
        q_dbg("tm_transfer : %s",tm_errorno_str(err_no));
    else
        q_dbg("                             (%d,%d)",x,y);
    tm_check_per(x,y,&tm->fb[f]);

#endif


    return 0;
}

#endif
