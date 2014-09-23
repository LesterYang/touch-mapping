/*
 *  tm_test.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : Test tools
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "tm_test.h"
#include "fbutils.h"
#include <qsi_ipc_client_lib.h>

// depend on panel[0-2] <-> kernel setting
static fb_data_t fb;
// depend on panel[0-2] <-> ap setting
static evt_data_t evt;

static fb_data_t fb_slave[PNL_NUM-1];

static int mode = MONO_AP;
static int matched = 0;

struct option long_opts[] = {
    {"master",              1, 0,   'p'},
    {"slave",               1, 0,   's'},
    {"ap",                  1, 0,   'a'},
    {0,                     0, 0,   0}
};

struct ipc_data{
        QSI_Channel *server;
        QSI_RECV_EVENT recv_func;
        QSI_PROTOCOL_ST status;
        char *name;
        char debug;
        char *line;
        char *target;
        unsigned char *msg;
        int  len;
}g_ipc;

typedef struct _cmd_append_t {
    char hdr;
    char panel;
    char pnl_start_pos_x;
    char pnl_start_pos_y;
    char pnl_width;
    char pnl_high;
    char ap;
    char ap_start_pos_x;
    char ap_start_pos_y;
    char ap_width;
    char ap_high;
} cmd_append_t;

typedef struct _cmd_clear_t {
    char hdr;
    char panel;
} cmd_clear_t;

typedef struct _tm_cmd_t {
    char len;
    union{
        unsigned char   data[16];
        cmd_append_t    append;
        cmd_clear_t     clear;
    };
} tm_cmd_t;

static void sig(int sig)
{
    close_framebuffer();
    fflush(stderr);
    printf("signal %d caught\n", sig);
    fflush(stdout);
    _exit(1);
}


void recv_event(const char *from, unsigned int len, unsigned char *msg)
{
    printf("recv len %d, from %s\n", len, from);
}

int open_ipc()
{
    int retry;

    g_ipc.status = PROTOCOL_IDLE;
    g_ipc.recv_func = recv_event;
    g_ipc.name = "QSIPL2";

    for(retry=0;;retry++)
    {
        g_ipc.server = qsi_open_channel(g_ipc.name, 0, 0);

        if(g_ipc.server != NULL)
            break;

        usleep(100000);

        if(retry == 3)
        {
            printf("open \"%s\" channel status timeout\n",g_ipc.name);
            return 1;
        }
    }
    // set receiving event callback function
    qsi_set_event(g_ipc.server, g_ipc.recv_func);

    return 0;
}

void tm_close_ipc()
{
    if(g_ipc.server)
    {
        qsi_close_channel(g_ipc.server);
        g_ipc.server = NULL;
    }
}

void send_ipc(tm_cmd_t* cmd)
{       
#if 0
  int i;
  printf("send ipc ");
  for (i = 0; i < cmd->len; i++) {
      printf("%d ",cmd->data[i]);
  }
  printf("\n");
#endif

   g_ipc.status=qsi_send_buffer(g_ipc.server, "QSIPL3", cmd->data, cmd->len, 0); 

   if(g_ipc.status!=PROTOCOL_ACK_OK)
       printf("send ipc error\n");
}

int select_fb(int ap)
{
    if (ap == 6)
        return PNL2_FB_NUM;
    else if(ap == 4)
        return PNL1_FB_NUM;
    else
        return PNL0_FB_NUM;
}

void init_fb_data(fb_data_t* fb_data)
{
    int i;
    
    memcpy(fb_data->dev, default_fb, sizeof(default_fb));
    memcpy(fb_data->pan, default_pan, sizeof(default_pan));

    for (i = 0; i < STR_NUM; i++)
        memset(fb_data->str[i], 0, MAX_STR_LEN);
}

void set_fb_path(fb_data_t* fb_data, int ap)
{
    fb_data->fb_num = select_fb(ap);
    fb_data->dev[FB_NUM_POS]= '0' + fb_data->fb_num;
    fb_data->pan[PAN_NUM_POS]= '0' + fb_data->fb_num;
}

void set_mono_comment()
{
    int i = PNL_NUM - 1, j;

    while(i)
    {   
        printf("set comment %d\n", i);
        i--;
        memcpy(fb_slave[i].str[0], "slave x", 7);
        memcpy(fb_slave[i].str[1], "display panel x screen", 22);

        fb_slave[i].str[0][6]  = fb_slave[i].pnl_id + '0';
        fb_slave[i].str[1][14] = fb.pnl_id + '0';
        
        if( !matched && fb.pnl_arg== fb_slave[i].pnl_id) 
            memcpy(fb_slave[i].str[2], "it's master", 11);
        else  
            memcpy(fb_slave[i].str[2], "-", 1);

        for(j = 3; j < STR_NUM; j++)
            memcpy(fb_slave[i].str[j], "-", 1);
    }
}

void set_de_comment()
{
}

void set_comment()
{
    switch(mode)
    {
        case MONO_AP:
            set_mono_comment();
            break;
        case DE_AP:
            set_de_comment();
            break;
        case TRI_AP:
        case TETRA_AP:
        case PENTA_AP:
        case HEXA_AP:
        case HEPTA_AP:
        case OCTA_AP:
        case NONA_AP:
        case DECA_AP:
        default:
                break;
    }
}

void set_fb()
{ 
    int i = PNL_NUM - 1;
    int pnl = fb.pnl_arg;
    int ap = fb.ap_arg[0];
    
    init_fb_data(&fb);
    set_fb_path(&fb, ap);

    while(i)
    {
        init_fb_data(&fb_slave[--i]);
    }
    
    matched = (pnl == 0 && (ap >=0 && ap <=3)) || 
              (pnl == 1 && ap == 4) || 
              (pnl == 2 && ap == 6);
    
    fb.ap_id = ap;
    fb.pnl_arg = pnl;

    switch(fb.fb_num)
    {
        case PNL0_FB_NUM:
            fb.pnl_id = 0;
            fb_slave[i].pnl_id = 1;
            set_fb_path(&fb_slave[i++], PNL1_DEFAULT_AP);
            fb_slave[i].pnl_id = 2;
            set_fb_path(&fb_slave[i++], PNL2_DEFAULT_AP);
            set_comment();
            break;
        case PNL1_FB_NUM:
            fb.pnl_id = 1;
            fb_slave[i].pnl_id = 0;
            set_fb_path(&fb_slave[i++], PNL0_DEFAULT_AP);
            fb_slave[i].pnl_id = 2;
            set_fb_path(&fb_slave[i++], PNL2_DEFAULT_AP);
            set_comment();
            break;
        case PNL2_FB_NUM:
            fb.pnl_id = 2;
            fb_slave[i].pnl_id = 0;
            set_fb_path(&fb_slave[i++], PNL0_DEFAULT_AP);
            fb_slave[i].pnl_id = 1;
            set_fb_path(&fb_slave[i++], PNL1_DEFAULT_AP);
            set_comment();
            break;
        default:
            return;
    }

    
#if 0
    open_slave_fb(&fb_slave[0]);
    open_slave_fb(&fb_slave[1]);

    refresh_slave_screen("slave0", &fb_slave[0]);
    refresh_slave_screen("slave1", &fb_slave[1]);

    close_slave_fb(&fb_slave[0]);
    close_slave_fb(&fb_slave[1]);
#endif
}

void set_evt()
{
    int ap = fb.ap_arg[0];
    
    memcpy(evt.dev, default_evt, sizeof(default_evt));
    evt.num = ap;
    evt.dev[EVT_NUM_POS] = '0' + evt.num;
}

int main(int argc, const char *argv[])
{ 
    tm_cmd_t cmd;
    char pnl=0x1F,ap=0x1F,i;
    int fd, opt_idx;
    char *short_opts = "p:s:a:";
    int c;

    memset((char*)&cmd, 0, sizeof(cmd));

    while ((c = getopt_long(argc, (char* const*)argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'a':
                ap = optarg[0] - '0';
                fb.ap_arg[0] = ap;
                printf("set ap %d\n", ap);
                break;
            case 'p':
                pnl = optarg[0] - '0';
                fb.pnl_arg = pnl;
                break;
            case 's':
                break;
            default:
                break;
        }
    }

    if(pnl==0x1F || ap==0x1F)
    {
        printf("error : need to set panel and ap number\n");
        return 0;
    }

    signal(SIGSEGV, sig);
    signal(SIGINT, sig);
    signal(SIGTERM, sig);

    set_fb();
    set_evt();
    
    printf("open pan : %s\n",fb.pan);
    printf("open fb  : %s\n",fb.dev);
    printf("open evt : %s\n",evt.dev);

    // set fb position and draw
    for (i = 0; i < PNL_NUM - 1; i++) 
    {
        if((fd=open(fb_slave[(int)i].pan, O_RDWR))>=0)
        {
            if (write(fd, "0,0", 3)<0) {
                dbg_log("write pan error, pan : %s",fb_slave[(int)i].pan);
            }
            close(fd);
        }
        else
        {
            printf("open pan error\n");
        }
        ts_test(&fb_slave[(int)i], NULL);
    }

    if((fd=open(fb.pan, O_RDWR))>=0)
    {
        if (write(fd, "0,0", 3)<0)  
            printf("write pan error\n");

        close(fd);
    }
    else
    {
        printf("open pan error\n");
    }

    pid_t pid = fork();
    
    if (pid == -1) 
    {
        printf("fork 1 error \n");
    } 
    else if (pid == 0) 
    {
         printf("fork 1\n");
        ap=4;
        evt.num = ap;
        evt.dev[EVT_NUM_POS] = '0' + evt.num;
        fb.fb_num = select_fb(ap);
        fb.dev[FB_NUM_POS]= '0' + fb.fb_num;
        fb.pan[PAN_NUM_POS]= '0' + fb.fb_num;
        ts_test(&fb, &evt);
        _exit(0);
    }

    pid = fork();

    if (pid == -1) 
    {
        printf("fork 2 error \n");
    } 
    else if (pid == 0) 
    {
        printf("fork 2\n");
        ap=6;
        evt.num = ap;
        evt.dev[EVT_NUM_POS] = '0' + evt.num;
        fb.fb_num = select_fb(ap);
        fb.dev[FB_NUM_POS]= '0' + fb.fb_num;
        fb.pan[PAN_NUM_POS]= '0' + fb.fb_num;
        ts_test(&fb, &evt);
        _exit(0);
    }

#if 1    
        open_ipc();
    
        // set ap display
        for (i = 0; i < 3; i++) 
        {
            cmd.clear.hdr=0xa1;
            cmd.clear.panel=i;
            cmd.len=2;
            send_ipc(&cmd);
    
            cmd.append.hdr=0xa0;
            cmd.append.panel=i;
            cmd.append.pnl_start_pos_x=0;
            cmd.append.pnl_start_pos_y=0;
            cmd.append.pnl_width=100;
            cmd.append.pnl_high=100;
            cmd.append.ap=ap;
            cmd.append.ap_start_pos_x=0;
            cmd.append.ap_start_pos_y=0;
            cmd.append.ap_width=100;
            cmd.append.ap_high=100;
            cmd.len=11;
           // send_ipc(&cmd);
        }
#endif


    // start test
    ap=0;
    evt.num = ap;
    evt.dev[EVT_NUM_POS] = '0' + evt.num;
    fb.fb_num = select_fb(ap);
    fb.dev[FB_NUM_POS]= '0' + fb.fb_num;
    fb.pan[PAN_NUM_POS]= '0' + fb.fb_num;
    ts_test(&fb, &evt);

    if(g_ipc.server)
    {
        qsi_close_channel(g_ipc.server);
        g_ipc.server = NULL;
    }

    return 0;
}
