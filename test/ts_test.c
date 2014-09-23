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
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "tslib.h"
#include "fbutils.h"
#include "tm_test.h"
#include "../include/tm.h"


extern fb_data_t fb_data[];
extern evt_data_t evt_data[];
 
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

	if (samp->pressure > 0) {
		if (inside) {
			if (!(button->flags & BUTTON_ACTIVE)) {
				button->flags |= BUTTON_ACTIVE;
				button_draw (button);
			}
		} else if (button->flags & BUTTON_ACTIVE) {
			button->flags &= ~BUTTON_ACTIVE;
			button_draw (button);
		}
	} else if (button->flags & BUTTON_ACTIVE) {
		button->flags &= ~BUTTON_ACTIVE;
		button_draw (button);
            dbg_log("");
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
    //memcpy(&str[sizeof(substr)], TM_VERSION, sizeof(TM_VERSION));
    memcpy(&str[20], TM_VERSION, sizeof(TM_VERSION));

	fillrect (0, 0, xres - 1, yres - 1, 0);
	put_string_center (xres/2, yres/3,   "test program", 1);
	put_string_center (xres/2, yres/3+20, str, 2);

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]);
}

int ts_test(fb_data_t* fb, evt_data_t* evt)
{
	struct tsdev *ts;
	int x, y, pos_y;
	unsigned int i;
	unsigned int mode = 0;
        int loop = 1;

        if (evt)
        {
            printf("%s\n",evt->dev);

            ts = ts_open (evt->dev, 0);

            if (!ts) {
                    perror (evt->dev);
                    exit(1);
            }

            if (ts_config(ts)) {
                    perror("ts_config");
                    exit(1);
            }
        }
        else
        {
            loop = 0;
        }

	if (open_framebuffer(fb)) {
		close_framebuffer();
		exit(1);
	}

	x = xres/2;
	y = yres/2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	memset (&buttons, 0, sizeof (buttons));

    for (i = 0; i < NR_BUTTONS; i++)
    {
		buttons [i].w = xres / 4;
        buttons [i].h = 20;
    }
    
    buttons [2].w = xres / 6;


	buttons [0].x = xres / 4 - buttons [0].w / 2 - 20;
	buttons [1].x = (3 * xres) / 4 - buttons [0].w / 2 + 20;
    buttons [2].x = xres / 2 - buttons [2].w / 2; 
    buttons [3].x = buttons [0].x;

    for (i = 4; i < NR_BUTTONS; i++)
        buttons [i].x = buttons [1].x;
    
	buttons [0].y = buttons [1].y = 10;
    buttons [2].y = 40;
	buttons [3].y = buttons [4].y = 70;

    for (i = 5, pos_y = 100; i < NR_BUTTONS; i++, pos_y += 30)
        buttons [i].y = pos_y;
	
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	
	if(loop)
    {
        buttons [2].text = "master";
        buttons [3].text = evt->dev;
        buttons [4].text = fb->dev;
        buttons [5].text = " ";
        buttons [6].text = " ";
        buttons [7].text = " ";
        buttons [8].text = " ";
    }
    else
    {
        printf("panel id   : %d\n",fb->pnl_id);
        for (i = 2; i < NR_BUTTONS; i++)
        {
            buttons [i].text = fb->str[i-2];
            printf("slave str %d : %s\n", i-2, buttons [i].text);
        }
    }


        
	refresh_screen ();

	while (loop) {
		struct ts_sample samp;
		int ret;

		/* Show the cross */
		if ((mode & 15) != 1)
			put_cross(x, y, 2 | XORMODE);

        printf("read %s\n",evt->dev);

		ret = ts_read(ts, &samp, 1);

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

		for (i = 0; i < NR_BUTTONS; i++)
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

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

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
