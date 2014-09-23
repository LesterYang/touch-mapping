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

struct test_conf{
    int pnl;
    int fb;
    int ap;
    int evt;
};

struct pos_conf{
    int pos_x;
    int pos_y;
    int width;
    int high;
};

struct test_data{
    fb_data_t  fb[PNL_NUM];
    evt_data_t evt[PNL_NUM];
    int   pnl_arg;
    int   ap_arg[MAX_AP_NUM];
    int   ap_num;
    int   mode;
    int   split;
};

static struct test_data ttm;

struct test_conf tconf[] = {
    {0, PNL0_FB_NUM, PNL0_DEFAULT_AP, PNL0_DEFAULT_EVT},
    {1, PNL1_FB_NUM, PNL1_DEFAULT_AP, PNL1_DEFAULT_EVT},
    {2, PNL2_FB_NUM, PNL2_DEFAULT_AP, PNL2_DEFAULT_EVT},
};

struct option long_opts[] = {
    {"master",              1, 0,   'p'},
    {"split",               0, 0,   's'},
    {"ap",                  1, 0,   'a'},
    {0,                     0, 0,   0}
};

struct pos_conf mono_conf[MAX_AP_NUM] = {
    {0,   0, 100, 100}
};
struct pos_conf de_conf[MAX_AP_NUM] = {
    {0,   0,  50, 100},
    {50,  0,  50, 100}
};
struct pos_conf tri_conf[MAX_AP_NUM] = {
    {0,   0,  33, 100},
    {33,  0,  34, 100},
    {37,  0,  33, 100}
};
struct pos_conf tetra_conf[MAX_AP_NUM] = {
    {0,   0,  50, 50},
    {50,  0,  50, 50},
    {0,  50,  50, 50},
    {50, 50,  50, 50}
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
#if 1
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

int get_fb(int pnl)
{
    return tconf[pnl].fb;
}
int get_default_ap(int pnl)
{
    return tconf[pnl].ap;
}
int get_default_evt(int pnl)
{
    return tconf[pnl].evt;
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

int select_pnl(int ap)
{
    if (ap == 6)
        return 2;
    else if(ap == 4)
        return 1;
    else
        return 0;
}

void set_comment()
{
    int i,j;

    for(i=0; i<PNL_NUM; i++)
    {   
        if(i == ttm.pnl_arg)
        {
            memcpy(ttm.fb[i].str[0], "master", 6);
            
            for(j=1; j<STR_NUM; j++)
            {
                if(j < ttm.ap_num + 1)
                {
                    int p;
                    sprintf(ttm.fb[i].str[j], "ap %d", ttm.ap_arg[j-1]);
                    if ((p = select_pnl(ttm.ap_arg[j-1])) != ttm.pnl_arg)
                    {
                        int pos = strlen(ttm.fb[i].str[j]);
                        sprintf(&ttm.fb[i].str[j][pos], "(see panel %d)", p);
                    }
                }
                else
                    memcpy(ttm.fb[i].str[j], " ", 1);
            }
        }
        else
        {
            sprintf(ttm.fb[i].str[0], "watch panel %d", ttm.pnl_arg);
            for(j=1; j<STR_NUM; j++)
                    memcpy(ttm.fb[i].str[j], " ", 1);
        }
    }
}

void init_fb(fb_data_t* fb, int pnl)
{
    int i;
    
    memcpy(fb->dev, default_fb, sizeof(default_fb));
    memcpy(fb->pan, default_pan, sizeof(default_pan));

    for (i = 0; i < STR_NUM; i++)
        memset(fb->str[i], 0, MAX_STR_LEN);

    fb->fb_id = get_fb(pnl);
    fb->dev[FB_NUM_POS]= '0' + fb->fb_id;
    fb->pan[PAN_NUM_POS]= '0' + fb->fb_id;
}


void init_evt(evt_data_t *evt, int pnl)
{
    memcpy(evt->dev, default_evt, sizeof(default_evt));

    evt->act = 0;
    evt->num = get_default_ap(pnl);
    evt->dev[EVT_NUM_POS] = '0' + evt->num;
}

void set_ttm()
{
    int i;
    for(i=0; i<PNL_NUM; i++)
    {
        init_evt(&ttm.evt[i], i);
        init_fb(&ttm.fb[i], i);
    }

    for(i=0; i<ttm.ap_num; i++)
        ttm.evt[select_pnl(ttm.ap_arg[i])].act = 1;
}

void set_pnl_append_cmd(cmd_append_t* a, int idx)
{
    //struct pos_conf (*pconf)[10] = NULL;
    void * pconf = NULL;

    if(idx > ttm.mode)
        return;
        
    switch(ttm.mode)
    {
        case MONO_AP:   pconf = &mono_conf;     break;
        case DE_AP:     pconf = &de_conf;       break;
        case TRI_AP:    pconf = &tri_conf;      break;
        case TETRA_AP:  pconf = &tetra_conf;    break;
        case PENTA_AP:
        case HEXA_AP:
        case HEPTA_AP:;
        case OCTA_AP:
        case NONA_AP:
        case DECA_AP:
        default:        break;
    }
    
    if(!pconf)
        return;

    a->pnl_start_pos_x = ((struct pos_conf (*)[MAX_AP_NUM])pconf)[idx]->pos_x;
    a->pnl_start_pos_y = ((struct pos_conf (*)[MAX_AP_NUM])pconf)[idx]->pos_y;
    a->pnl_width       = ((struct pos_conf (*)[MAX_AP_NUM])pconf)[idx]->width;
    a->pnl_high        = ((struct pos_conf (*)[MAX_AP_NUM])pconf)[idx]->high;           
}

void set_ap_append_cmd(cmd_append_t* a, int idx)
{
    if(!ttm.split)
    {
        a->ap_start_pos_x=0;
        a->ap_start_pos_y=0;
        a->ap_width=100;
        a->ap_high=100;
        return;
    }
    
    //struct pos_conf (*pconf)[MAX_AP_NUM] = NULL;
    void * pconf = NULL;

    if(idx > ttm.mode)
        return;
        
    switch(ttm.mode)
    {
        case MONO_AP:   pconf = &mono_conf;     break;
        case DE_AP:     pconf = &de_conf;       break;
        case TRI_AP:    pconf = &tri_conf;      break;
        case TETRA_AP:  pconf = &tetra_conf;    break;
        case PENTA_AP:
        case HEXA_AP:
        case HEPTA_AP:;
        case OCTA_AP:
        case NONA_AP:
        case DECA_AP:
        default:        break;
    }
    
    if(!pconf)
        return;

    a->ap_start_pos_x = ((struct pos_conf [MAX_AP_NUM])*pconf)[idx]->pos_x;
    a->ap_start_pos_y = ((struct pos_conf [MAX_AP_NUM])*pconf)[idx]->pos_y;
    a->ap_width       = ((struct pos_conf [MAX_AP_NUM])*pconf)[idx]->width;
    a->ap_high        = ((struct pos_conf [MAX_AP_NUM])*pconf)[idx]->high;   

}

int main(int argc, const char *argv[])
{ 
    tm_cmd_t cmd;
    char i;
    int fd, opt_idx, c;
    char *short_opts = "p:sa:";
    pid_t pid;

    memset((char*)&cmd, 0, sizeof(cmd));

    while ((c = getopt_long(argc, (char* const*)argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'a':
                ttm.ap_arg[ttm.ap_num] = optarg[0] - '0';
                ttm.ap_num++;
                break;
            case 'p':
                ttm.pnl_arg = optarg[0] - '0';
                break;
            case 's':
                ttm.split = 1;
                break;
            default:
                break;
        }
    }

    if(ttm.pnl_arg < 0 || ttm.pnl_arg > PNL_NUM)
    {
        printf("set panel error\n");
        return 0;
    }    

    if(!ttm.ap_num)
    {
        printf("no ap id\n");
        return 0;
    }

    signal(SIGSEGV, sig);
    signal(SIGINT, sig);
    signal(SIGTERM, sig);

    ttm.mode =  ttm.ap_num - 1;

    set_ttm();
    set_button_num(ttm.ap_num + 3);
    set_comment();

    // set fb position
    for (i = 0; i < PNL_NUM; i++) 
    {
        printf("panel %d pan : %s\n",i,ttm.fb[(int)i].pan);
        printf("panel %d fb  : %s\n",i,ttm.fb[(int)i].dev);
        printf("panel %d evt : %s\n",i,ttm.evt[(int)i].dev);
        if((fd=open(ttm.fb[(int)i].pan, O_RDWR))>=0)
        {
            if (write(fd, "0,0", 3)<0) {
                dbg_log("write pan error, pan : %s",ttm.fb[(int)i].pan);
            }
            close(fd);
        }
        else
        {
            printf("open pan error\n");
        }
#if 1      
        pid = fork();
        if (pid == -1) 
        {
            printf("fork %d error \n", i);
        }
        else if (pid == 0)
        {
            ts_test(&ttm.fb[(int)i], &ttm.evt[(int)i]);
            _exit(0);
        }
#endif
    }
  
    open_ipc();

    // set ap display
    for (i = 0; i < 3; i++) 
    {
        cmd.clear.hdr=0xa1;
        cmd.clear.panel=i;
        cmd.len=2;
        
        send_ipc(&cmd);
  
        for(int j=0; j<ttm.ap_num; j++)
        {
            cmd.append.hdr=0xa0;
            cmd.append.panel=i;
            cmd.append.ap=ttm.ap_arg[j];

            set_pnl_append_cmd(&cmd.append, j);
            set_ap_append_cmd(&cmd.append, j);
            
            cmd.len=11;
            
            send_ipc(&cmd);
        }
    }

    if(g_ipc.server)
    {
        qsi_close_channel(g_ipc.server);
        g_ipc.server = NULL;
    }

    return 0;
}
