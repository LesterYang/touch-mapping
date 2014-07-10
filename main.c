/*
 * main.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */
#include "stdlib.h"
#include "qts.h"

void tsqsi_check_per(short x, short y, struct tsqsi_dev* dev)
{
    int perx = (((int)x - dev->min_x) * MULTIPLE) / (dev->max_x - dev->min_x);
    int pery = (((int)y - dev->min_y) * MULTIPLE) / (dev->max_y - dev->min_y);
    printf("x %2d%% y %2d%%\n",(perx*100)/MULTIPLE, (pery*100)/MULTIPLE);
}

int main(int argc, char* argv[])
{
    struct tsqsi* ts;
    qerrno err_no;

    if((err_no = tsqsi_create(&ts)) != eENoErr)
        printf("tsqsi_create : %s\n",tsqsi_errorno_str(err_no));

#if 1 // test points
    short x,y;
    unsigned char p,f;

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

    tsqsi_check_per(x,y,&ts->panel[p]);
    printf("panel%d to fb%d : (%d,%d) -> ",p,f,x,y);
    if((err_no = tsqsi_transfer(&x, &y, ts, p, f))!= eENoErr)
        printf("\ntsqsi_transfer : %s\n",tsqsi_errorno_str(err_no));
    else
        printf("(%d,%d)\n",x,y);
    tsqsi_check_per(x,y,&ts->fb[f]);

#endif

    tsqsi_destroy(ts);
    return 0;
}
