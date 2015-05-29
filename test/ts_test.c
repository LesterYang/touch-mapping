/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_test.c,v 1.6 2004/10/19 22:01:27 dlowder Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "tslib.h"
#include "fbutils.h"
#include "tm_test.h"
#include "../daemon/include/tm.h"

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif

 
static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

struct ts_button {
	int x, y, w, h;
	char *text;
	int flags;
#define BUTTON_ACTIVE 0x00000001
};

/* [inactive] border fill text [active] border fill text */
static int button_palette [6] =
{
	1, 4, 2,
	1, 5, 0
};

static struct ts_button buttons [NR_BUTTONS];
static int button_num = 2;

void set_button_num(int num)
{
    button_num = num;
}

static void button_draw (struct ts_button *button)
{
	int s = (button->flags & BUTTON_ACTIVE) ? 3 : 0;
	rect (button->x, button->y, button->x + button->w - 1,
	      button->y + button->h - 1, button_palette [s]);
	fillrect (button->x + 1, button->y + 1,
		  button->x + button->w - 2,
		  button->y + button->h - 2, button_palette [s + 1]);
	put_string_center (button->x + button->w / 2,
			   button->y + button->h / 2,
			   button->text, button_palette [s + 2]);
}

static int button_handle (struct ts_button *button, struct ts_sample *samp)
{
    int inside = (samp->x >= button->x) && (samp->y >= button->y) &&
		(samp->x < button->x + button->w) &&
		(samp->y < button->y + button->h);
    
    if (samp->pressure > 0) 
    {
        if (inside)
        {
	    if (!(button->flags & BUTTON_ACTIVE))
	    {
                button->flags |= BUTTON_ACTIVE;
                button_draw (button);
            }
        }
        else if (button->flags & BUTTON_ACTIVE)
        {
            button->flags &= ~BUTTON_ACTIVE;
            button_draw (button);
        }
    }
    else if (button->flags & BUTTON_ACTIVE)
    {
        button->flags &= ~BUTTON_ACTIVE;
        button_draw (button);
        dbg_log(" ");
        return 1;
    }

    return 0;
}

static void refresh_screen ()
{
	int i;
    
    char substr[]="tm-daemon version : ";
    char str[32]={0};

    memcpy(str, substr, sizeof(substr));
    memcpy(&str[20], TM_TEST_VERSION, sizeof(TM_TEST_VERSION));

	fillrect (0, 0, xres - 1, yres - 1, 0);
	put_string_center (xres/2, yres/4,   "test program", 1);
	put_string_center (xres/2, yres/4+20, str, 2);

	for (i = 0; i < button_num; i++)
		button_draw (&buttons [i]);
}

void set_button(fb_data_t* fb)
{
    int i, pos_x[STR_NUM], pos_y[STR_NUM];
    int mode = button_num - 4;
    int w = xres / 4, h = 20;

    /* Initialize buttons */
    memset (&buttons, 0, sizeof (buttons));

    /* set size and text */
    buttons [0].w = buttons [1].w = w;
    buttons [0].h = buttons [1].h = h;

    buttons [0].text = "Drag";
    buttons [1].text = "Draw";

    for (i = 0; i < STR_NUM; i++)
    {
        buttons [i+2].w = w;
        buttons [i+2].h = h;
        buttons [i+2].text = fb->str[i];

       
    }
    buttons [2].w = xres / 6 + 20;

    /* set buttons position */
    buttons [0].x = xres / 4 - w / 2 - 20;
    buttons [1].x = (3 * xres) / 4 - w / 2 + 20;
    buttons [0].y = buttons [1].y = 10;

    pos_x[0] = xres / 2 - buttons [2].w / 2;
    pos_y[0] = 40;

    switch(mode)
    {
        case MONO_AP:
            pos_x[1] = xres / 2 - w / 2;
            pos_y[1] = yres / 2;
            break;
        case DE_AP:
            pos_x[1] = xres / 4 - w / 2 - 20;
            pos_y[1] = yres / 2 - h / 2;
            pos_x[2] = (3 * xres) / 4 - w / 2 + 20;
            pos_y[2] = yres / 2 - h / 2;
            break;
        case TRI_AP:
            pos_x[1] = xres / 6 - w / 2;
            pos_y[1] = yres / 2 - h / 2;
            pos_x[2] = xres / 2 - w / 2;
            pos_y[2] = yres / 2 - h / 2;
            pos_x[3] = (5 * xres) / 6 - w / 2;
            pos_y[3] = yres / 2 - h / 2;
            break;
        case TETRA_AP:
            pos_x[1] = xres / 4 - w / 2 - 20;
            pos_y[1] = yres / 4 - h / 2;
            pos_x[2] = (3 * xres) / 4 - w / 2 + 20;
            pos_y[2] = yres / 4 - h / 2;
            pos_x[3] = xres / 4 - w / 2 - 20;
            pos_y[3] = (3 * yres) / 4 - h / 2;
            pos_x[4] = (3 * xres) / 4 - w / 2 + 20;
            pos_y[4] = (3 * yres) / 4 - h / 2;
            break;
        case PENTA_AP:
            break;
        case HEXA_AP:
            break;
        case HEPTA_AP:
            break;
        case OCTA_AP:
            break;
        case NONA_AP:
            break;
        case DECA_AP:
            break;
        default:
            break;
    }

    for (i = 0; i < STR_NUM; i++)
    {
        buttons [i+2].x = pos_x[i];
        buttons [i+2].y = pos_y[i];  
    } 
}

int tm_read_event(int fd, struct ts_sample *samp)
{
    int r;
    static struct input_event evt;
    static int slot;
    int reading = 1;
    
    while(reading)
    {
        r = read(fd, &evt, sizeof(struct input_event));

        if(r != sizeof(struct input_event))
            return -1;

        if(evt.type == EV_SYN)
            break;

        if(slot)
            continue;
    
        switch (evt.type)
        {
            case EV_ABS:
                switch (evt.code)
                {
                    case ABS_X:
                    case ABS_MT_POSITION_X:
                       samp->x = evt.value; 
                       break;
                    case ABS_Y:
                    case ABS_MT_POSITION_Y:
                        samp->y = evt.value; 
                        break;
                    case ABS_MT_SLOT:
                        slot = evt.value;
                        break;
                    default:
                        break;
                }
                break;   
            default:
                break;
        }
    }
    return 1;
}


#define USE_TS (0)
int ts_test(fb_data_t* fb, evt_data_t* evt)
{
#if USE_TS
    struct tsdev *ts = NULL;
#else
    int fd;
#endif
    int x, y;
    unsigned int i;
    unsigned int mode = 0;

    if (evt->act)
    {
#if USE_TS    
        ts = ts_open (evt->dev, 0);

        if (!ts) {
            perror (evt->dev);
            exit(1);
        }

        if (ts_config(ts)) {
            perror("ts_config");
            exit(1);
        }
#else
        fd = open(evt->dev, O_RDONLY);
        if (fd<0) {
            perror (evt->dev);
            exit(1);
        }
#endif
    }

    if (open_framebuffer(fb)) {
        close_framebuffer();
        exit(1);
    }

    x = xres/2;
    y = yres/2;

    for (i = 0; i < NR_COLORS; i++)
        setcolor (i, palette [i]);

    set_button(fb);      
    refresh_screen ();

    while (evt->act) {
        struct ts_sample samp;
        int ret;

        /* Show the cross */
        if ((mode & 15) != 1)
            put_cross(x, y, 2 | XORMODE);
#if USE_TS 
        ret = ts_read(ts, &samp, 1);
#else
        ret = tm_read_event(fd, &samp);
#endif
        /* Hide it */
        if ((mode & 15) != 1)
            put_cross(x, y, 2 | XORMODE);

        if (ret < 0) {
            perror("ts_read");
            close_framebuffer();
            exit(1);
        }

        if (ret != 1)
            continue;

        for (i = 0; i < button_num; i++)
            if (button_handle (&buttons [i], &samp))
                switch (i) {
                    case 0:
                        mode = 0;
                        refresh_screen ();
                        break;
                    case 1:
                        mode = 1;
                        refresh_screen ();
                        break;
                }

        printf("tm-test : read %18s -> %5d %5d\n", evt->dev, samp.x, samp.y);

        if (samp.pressure > 0) {
            if (mode == 0x80000001)
                line (x, y, samp.x, samp.y, 2);
            x = samp.x;
            y = samp.y;
            mode |= 0x80000000;
        } else
            mode &= ~0x80000000;

    }
    close_framebuffer();
    return 0;
}

static void refresh_conf_screen (char* string, char* substr)
{
    int i;
    
    fillrect (0, 0, xres - 1, yres - 1, 0);
    put_string_center (xres/2, yres/4, string, 1);
    put_string_center (xres/2, yres/4+20, substr, 2);

    for (i = 0; i < button_num; i++)
        button_draw (&buttons [i]);
}
void set_conf_button()
{
    int w = xres / 8, h = 50;

    /* Initialize buttons */
    memset (&buttons, 0, sizeof (buttons));

    /* set size and text */
    buttons [0].w = buttons [1].w = w;
    buttons [0].h = buttons [1].h = h;

    buttons [0].text = "Yes";
    buttons [1].text = "No";

    /* set buttons position */
    buttons [0].x = xres / 4 - w / 2 - 20;
    buttons [1].x = (3 * xres) / 4 - w / 2 + 20;
    buttons [0].y = buttons [1].y = 40;
}

#if USE_TS
int replace_config(fb_data_t* fb, evt_data_t* evt)
{
    struct tsdev *ts;
    unsigned int i;

    ts = ts_open (evt->dev, 0);

    if (!ts) {
        perror (evt->dev);
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

    button_num = 2;

    for (i = 0; i < NR_COLORS; i++)
        setcolor (i, palette [i]);

    set_conf_button();      
    refresh_conf_screen("CHECK", "Use new calibrated configuration?");

    while (1) {
        struct ts_sample samp;
        int ret;

        ret = ts_read(ts, &samp, 1);

        if (ret < 0) 
        {
            perror("ts_read");
            close_framebuffer();
            exit(1);
        }

        if (ret != 1)
            continue;

        for (i = 0; i < button_num; i++)
        {
            if (button_handle (&buttons [i], &samp))
            {
                switch (i)
                {
                    case 0:
                        refresh_conf_screen ("CHECK", "Press Yes");
                        break;
                    case 1:
                        refresh_conf_screen ("CHECK", "Press No");
                        break;
                    default:
                        break;
                }
            }
        }
    }
    ts_close(ts);
    close_framebuffer();
    return 0;
}
#endif
int refresh_tm_test(fb_data_t* fb)
{
    unsigned int i;

    if (open_framebuffer(fb)) {
        close_framebuffer();
        exit(1);
    }

    for (i = 0; i < NR_COLORS; i++)
        setcolor (i, palette [i]);

    refresh_conf_screen("All of Panel Is Calibrated", "Restart tm-daemon Or Reboot");

    close_framebuffer();
    return 0;
}

