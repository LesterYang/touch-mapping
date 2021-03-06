/*
 *  
 *  tslib/tests/ts_calibrate.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_calibrate.c,v 1.8 2004/10/19 22:01:27 dlowder Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tslib.h"

#include "fbutils.h"
#include "testutils.h"
#include "tm_test.h"


#define TMP_CFG_FILE    "/tmp/calpoint"
#define TEST_CAL        "cal_conf"
#define TEST_CAL_SIZE   (8)
#define CAL_ID_POS      (TEST_CAL_SIZE+1)
#define CAL_W_POS       (CAL_ID_POS+2) 
#define BUF_LEN         (255)

static char calpoint[MAX_PNL_NUM][128];
static int calnum;
static char cfg_name[1024];


static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

static void sig(int sig)
{
	close_framebuffer ();
	fflush (stderr);
	printf ("signal %d caught\n", sig);
	fflush (stdout);
	exit (1);
}

int perform_calibration(calibration *cal) {
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
		printf("ts_calibrate: determinant is too small -- %f\n",det);
		return 0;
	}

// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer x coord
	cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer y coord
	cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// If we got here, we're OK, so assign scaling to a[6] and return
	cal->a[6] = (int)scaling;
	return 1;
/*	
// This code was here originally to just insert default values
	for(j=0;j<7;j++) {
		c->a[j]=0;
	}
	c->a[1] = c->a[5] = c->a[6] = 1;
	return 1;
*/

}

static void get_sample (struct tsdev *ts, calibration *cal,
			int index, int x, int y, char *name)
{
	static int last_x = -1, last_y;
	if (last_x != -1) {
#define NR_STEPS 10
		int dx = ((x - last_x) << 16) / NR_STEPS;
		int dy = ((y - last_y) << 16) / NR_STEPS;
		int i;
		last_x <<= 16;
		last_y <<= 16;
		for (i = 0; i < NR_STEPS; i++) {
			put_cross (last_x >> 16, last_y >> 16, 2 | XORMODE);
			usleep (1000);
			put_cross (last_x >> 16, last_y >> 16, 2 | XORMODE);
			last_x += dx;
			last_y += dy;
		}
	}

	put_cross(x, y, 2 | XORMODE);
	getxy (ts, &cal->x [index], &cal->y [index]);
	put_cross(x, y, 2 | XORMODE);

	last_x = cal->xfb [index] = x;
	last_y = cal->yfb [index] = y;

	printf("%s : X = %4d Y = %4d\n", name, cal->x [index], cal->y [index]);
}

int ts_cal(fb_data_t* fb, char* evt_path)
{
    struct tsdev *ts;
    calibration cal;
    unsigned int i;

    signal(SIGSEGV, sig);
    signal(SIGINT, sig);
    signal(SIGTERM, sig);

    ts = ts_open(evt_path,0);

    if (!ts) {
        perror("ts_open");
        exit(1);
    }
    if (ts_config(ts)) {
        perror("ts_config");
        exit(1);
    }

    if (open_framebuffer(fb)) {
        close_framebuffer();
        exit(1);
    }

    for (i = 0; i < NR_COLORS; i++)
        setcolor (i, palette [i]);

    put_string_center (xres / 2, yres / 4,
                       "tm-daemon calibration", 1);
    put_string_center (xres / 2, yres / 4 + 20,
                       "Touch crosshair to calibrate", 2);
    printf("xres = %d, yres = %d\n", xres, yres);

// Read a touchscreen event to clear the buffer
//	getxy(ts, 0, 0);
    get_sample (ts, &cal, 0, 50,        50,        "Top left");
    get_sample (ts, &cal, 1, xres - 50, 50,        "Top right");
    get_sample (ts, &cal, 2, xres - 50, yres - 50, "Bot right");
    get_sample (ts, &cal, 3, 50,        yres - 50, "Bot left");
    get_sample (ts, &cal, 4, xres / 2,  yres / 2,  "Center");

    if (perform_calibration (&cal)) {
        fprintf (stderr,"Calibration constants: ");
        //fprintf (stderr,"%d %d %d %d %d %d %d\n",
        //         cal.a[1], cal.a[2], cal.a[0],
        //         cal.a[4], cal.a[5], cal.a[3], cal.a[6]);
        i = 0;
        sprintf (calpoint[calnum++], "%d %d %d %d %d %d %d ",
                                    cal.a[1], cal.a[2], cal.a[0],
                                    cal.a[4], cal.a[5], cal.a[3], cal.a[6]);

    } else {
        printf("Calibration failed.\n");
        i = -1;
    }

    fillrect (0, 0, xres - 1, yres - 1, 0);
    put_string_center (xres/2, yres/4,   "   Finish calibration   ", 1);
    put_string_center (xres/2, yres/4+20, "   Please calibrate next panel   ", 2);
   
    ts_close(ts);

    return i;
}

int get_cfg_path()
{
    char proc[32]={0};
    char path[1024]={0};
    char *p;
    
    sprintf(proc, "/proc/%d/exe", getpid());

    if(readlink(proc, path, 1024)<0)
        return -1;

    //get lst_tm version folder
    if(!(p=strrchr(path,'/')))
        return -1;
    *p=0;

    // get lst_tm folder
    if(!(p=strrchr(path,'/')))
        return -1;
    *p=0;

    sprintf(cfg_name, "%s/lst_tm.conf", path);

    if(access(cfg_name, R_OK) != 0)
    {
        dbg_log("no %s or can't read this file", cfg_name);
        return -1;
    }
    
    return 0;
}



void update_calibrate()
{
    int i;
    FILE *fw,*fr;
    char buf[BUF_LEN]={0};
    char sh[BUF_LEN]={0};
    char *cfg;

    if(get_cfg_path() < 0)
        cfg = TMP_CFG_FILE;
    else
        cfg = cfg_name;

    if(!(fr=fopen(cfg, "r")))
    {
        printf("open %s error\n",cfg);
        return;
    }

    if(!(fw=fopen(TMP_CFG_FILE,"w")))
    {
        printf("open %s error\n",TMP_CFG_FILE);
        return;
    }
    
    for (i = 0; i < calnum; i++) {
        printf("update : %s \n", calpoint[i]);
    }

    while(!feof(fr))
    {
        for(i=0; i<BUF_LEN-1; i++)
        {
            fscanf(fr, "%c", &buf[i]);
            if(buf[i]==0xa)
            {
                buf[i+1]=0x0;
                break;
            }
        }

        if(memcmp(buf, TEST_CAL, TEST_CAL_SIZE) == 0)
        {
            int id = buf[CAL_ID_POS] - '0';

            if((calpoint[id][0] || calpoint[id][1]) && id<calnum)
            {
                int size = strlen(calpoint[id]);
                memcpy(&buf[CAL_W_POS], calpoint[id], size);
                buf[CAL_W_POS + size] = 0xa;
                buf[CAL_W_POS + size + 1] = 0;
            }
        }

       //printf("read : %s",buf);

        fprintf(fw, "%s", buf);
        sync();    
        memset(buf, 0, BUF_LEN);     
    }

    fclose(fr);
    fclose(fw);
    
    sprintf(sh, "cp %s %s.bak", cfg, cfg);
    system(sh);

    memset(sh, 0, BUF_LEN);
    sprintf(sh, "cp %s %s", TMP_CFG_FILE, cfg);
    system(sh);
    sync();
}


