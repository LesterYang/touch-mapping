/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "qUtils.h"
#include "tm.h"
#include "tmMap.h"
#include "tmInput.h"

struct sTmData* tm = NULL;
tm_main_status g_status = eTmStatusNone;
uint8_t  g_header[Queue_HdrLen]={Queue_Hdr0, Queue_Hdr1};


struct sStatusInfo{
	tm_main_status status;
	const char* str;
};

struct sStatusInfo statusInfo[] = {
    {eTmStatusNone,			"None"},
	{eTmStatusInputeInit,	"InputeInit"},
	{eTmStatusSocketInit,	"SocketInit"},
	{eTmStatusLoop,			"MainLoop"},
	{eTmStatusDeinit,		"Deinit"},
	{eTmStatusError,		"Error"},
};

#if 1 // test on ubuntu

struct sEventDev srcEvDev[] = {
    { "/dev/input/event0",       -1,    eTmDevFront,    eTmApQSI,       NULL,   {NULL} },
    { "/dev/input/event1",       -1,    eTmDevFront,    eTmApMonitor,   NULL,   {NULL} },
    { "/dev/input/event2",       -1,    eTmDevFront,    eTmApNavi,      NULL,   {NULL} },
    { "/dev/input/mouse0",       -1,    eTmDevLeft,     eTmApQSILeft,   NULL,   {NULL} },
    { "/dev/input/mouse1",       -1,    eTmDevRight,    eTmApQSIRight,  NULL,   {NULL} },
    {                NULL,        0,    eTmDevNone,     eTmApNone,      NULL,   {NULL} }
};

struct sEventDev destEvDev[] = {
    { "/dev/null",       -1,    eTmDevFront,    eTmApQSI,      NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevFront,    eTmApMonitor,  NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevFront,    eTmApNavi,     NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevLeft,     eTmApQSILeft,  NULL,   {NULL} },
    { "/dev/null",       -1,    eTmDevRight,    eTmApQSIRight, NULL,   {NULL} },
    {        NULL,        0,    eTmDevNone,     eTmApNone,     NULL,   {NULL} }
};

#else

struct sEventDev srcEvDev[] = {
    { "/dev/input/event0",       -1,    eTmDevFront,    eTmApQSI,       NULL,   {NULL} },
    { "/dev/input/event1",       -1,    eTmDevFront,    eTmApMonitor,   NULL,   {NULL} },
    { "/dev/input/event2",       -1,    eTmDevFront,    eTmApNavi,      NULL,   {NULL} },
    { "/dev/input/event5",       -1,    eTmDevLeft,     eTmApQSILeft,   NULL,   {NULL} },
    { "/dev/input/event6",       -1,    eTmDevRight,    eTmApQSIRight,  NULL,   {NULL} },
    {                NULL,        0,    eTmDevNone,     eTmApNone,      NULL,   {NULL} }
};

struct sEventDev destEvDev[] = {
    { "/dev/input/event20",       -1,    eTmDevFront,    eTmApQSI,      NULL,   {NULL} },
    { "/dev/input/event21",       -1,    eTmDevFront,    eTmApMonitor,  NULL,   {NULL} },
    { "/dev/input/event22",       -1,    eTmDevFront,    eTmApNavi,     NULL,   {NULL} },
    { "/dev/input/event25",       -1,    eTmDevLeft,     eTmApQSILeft,  NULL,   {NULL} },
    { "/dev/input/event26",       -1,    eTmDevRight,    eTmApQSIRight, NULL,   {NULL} },
    {                 NULL,        0,    eTmDevNone,     eTmApNone,     NULL,   {NULL} }
};
#endif



void tm_check_per(short x, short y, struct sTmDataDev* dev)
{
    int perx = (((int)x - dev->min_x) * MULTIPLE) / (dev->max_x - dev->min_x);
    int pery = (((int)y - dev->min_y) * MULTIPLE) / (dev->max_y - dev->min_y);
    printf("x %2d%% y %2d%%\n",(perx*100)/MULTIPLE, (pery*100)/MULTIPLE);
}

void tm_switch_main_status(tm_main_status status)
{
	q_dbg("status : %10s -> %s", statusInfo[g_status].str, statusInfo[status].str);
	g_status = status;

}

qerrno tm_init()
{
    int i, j;
    qerrno err_no;

    signal(SIGINT, tm_shutdown);
    signal(SIGHUP, tm_shutdown);

    if((err_no = tm_create(&tm)) != eENoErr)
    {
        q_dbg("tm_create : %s", tm_errorno_str(err_no));
        return err_no;
    }

    if(tm_inputInit(srcEvDev))
    	return eEOpen;

    for(i=0; destEvDev[i].evDevPath != NULL; i++)
    {
        if((destEvDev[i].fd = open (destEvDev[i].evDevPath, O_RDONLY )) < 0)
            continue;

        q_dbg("Opened %s as target device [counter %d]",destEvDev[i].evDevPath, i);

        for(j=0; srcEvDev[j].evDevPath != NULL; j++)
        {
            if(srcEvDev[j].ap == destEvDev[i].ap)
            {
                srcEvDev[j].targetDev = &destEvDev[i];
                destEvDev[j].sourceDev = &srcEvDev[i];
            }
        }
    }

    tm_set_dev_param(srcEvDev, tm->panel);
    tm_set_dev_param(destEvDev, tm->fb);

    return eENoErr;
}

void tm_deinit()
{
    int idx;
    tm_destroy(tm);

    for(idx=0; destEvDev[idx].evDevPath != NULL; idx++)
    {
        if (destEvDev[idx].fd >= 0)
        {
            q_close(destEvDev[idx].fd);
            q_dbg("Closed target device [counter %d]", idx);
        }
    }

    tm_inputDeinit();
}

void tm_shutdown(int signum)
{
    tm_deinit();
    exit(signum);
}

void tm_set_dev_param(struct sEventDev* evDev, struct sTmDataDev* dataDev)
{
    int i;
    tm_dev dev;

    for(i=0; evDev[i].evDevPath != NULL; i++)
    {
        dev = evDev[i].dev;

        if( dev >= 0 && dev < eTmDevMax)
            evDev[i].paramDev = &dataDev[dev];
    }
}

void tm_save_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op)
{
    q_dbg("save %d => type : %2x, code : %2x, value : %2x",srcEv->dev ,ev->type, ev->code, ev->value);

	if(g_status != eTmStatusLoop)
		return;

    q_set_queue(tm->queue, g_header, sizeof(g_header), q_true);
    q_set_queue(tm->queue, ev, sizeof(sInputEv), q_true);
    q_set_queue(tm->queue, &srcEv, sizeof(struct sEventDev *), q_true);
    q_set_queue(tm->queue, &op, sizeof(tm_op_code), q_true);

    return;
}

void tm_send_event(sInputEv *ev, struct sEventDev *srcEv, tm_op_code op)
{
    q_dbg("send %d => type : %2x, code : %2x, value : %2x",srcEv->dev ,ev->type, ev->code, ev->value);
    return;
}

void tm_parse_event()
{
    char* buf;
    char hdr;
    tm_op_code op;
    struct sEventDev *srcEv;
    sInputEv ev;

    while(q_size_queue(tm->queue) != 0)
    {
        if( q_pop_queue(tm->queue, &hdr) || hdr != (char)Queue_Hdr0 )
            continue;

        if( q_pop_queue(tm->queue, &hdr) || hdr != (char)Queue_Hdr1 )
            continue;

        buf = (char*)&ev;
        if(q_get_queue(tm->queue, buf, sizeof(sInputEv)) != sizeof(sInputEv))
            break;

        buf = (char*)&srcEv;
        if(q_get_queue(tm->queue, buf, sizeof(struct sEventDev *)) != sizeof(struct sEventDev *))
            break;

        buf = (char*)&op;
        if(q_get_queue(tm->queue, buf, sizeof(tm_op_code)) != sizeof(tm_op_code))
            break;

        tm_send_event(&ev, srcEv, op);
    }
}

int main(int argc, char* argv[])
{

// fork ....

	int count = 0; // for test
	g_status = eTmStatusInputeInit;

	while(1)
	{
		switch(g_status)
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
			    while((++count) < 50)
			    {
			        if(q_size_queue(tm->queue) != 0)
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
				break;

			case eTmStatusError:
				tm_switch_main_status(eTmStatusDeinit);
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
