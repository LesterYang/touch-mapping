/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "qUtils.h"
#include "tmMap.h"
#include "tmInput.h"

struct sTmData* tm;

void tm_init();
void tm_deinit();
void tm_shutdown(int signum);

void tm_check_per(short x, short y, struct sTmData_dev* dev)
{
    int perx = (((int)x - dev->min_x) * MULTIPLE) / (dev->max_x - dev->min_x);
    int pery = (((int)y - dev->min_y) * MULTIPLE) / (dev->max_y - dev->min_y);
    printf("x %2d%% y %2d%%\n",(perx*100)/MULTIPLE, (pery*100)/MULTIPLE);
}

void tm_shutdown(int signum)
{
    tm_deinit();
    exit(signum);
}

void tm_init()
{
    qerrno err_no;

    signal(SIGINT, tm_shutdown);
    signal(SIGHUP, tm_shutdown);

    if((err_no = tm_create(&tm)) != eENoErr)
        q_dbg("tm_create : %s", tm_errorno_str(err_no));

    tm_inputInit();
}

void tm_deinit()
{
    tm_destroy(tm);
    tm_inputDeinit();
}

int main(int argc, char* argv[])
{

// fork ....

    tm_init();
// socket init ...
// while loop...

#if 1 // test points
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
        y = 700;
        p = 4;
        f = 0;
    }

    tm_check_per(x,y,&tm->panel[p]);
    q_dbg("panel%d to fb%d : (%d,%d) -> ",p,f,x,y);
    if((err_no = tm_transfer(&x, &y, tm, p, f))!= eENoErr)
        q_dbg("tm_transfer : %s",tm_errorno_str(err_no));
    else
        q_dbg("                             (%d,%d)",x,y);
    tm_check_per(x,y,&tm->fb[f]);

#endif
    sleep(5);

//  close socket ..
    tm_deinit();

    return 0;
}
