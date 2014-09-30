/*
 *  tm_test.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : Test tools
 */
 
#include <stdio.h>
#include <stdlib.h>
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
    int org_evt;
};

struct pos_mode{
    int pos_x;
    int pos_y;
    int width;
    int high;
};

struct test_data{
    int   pnl_arg;
    int   pnl_num_arg;
    int   ap_arg[MAX_AP_NUM];
    int   fb_arg[MAX_PNL_NUM];
    int   evt_arg[MAX_PNL_NUM];
    int   org_evt_arg[MAX_PNL_NUM];

    fb_data_t  fb[PNL_NUM];
    evt_data_t evt[PNL_NUM];
    int   ap_num;
    int   fb_num;
    int   evt_num;
    int   org_evt_num;
    int   mode;

    int   split;
    int   wait_ver;
    int   calibrate;

    char  set_pnl_num;
    char  set_ap;
    char  set_fb;
    char  set_evt;
    char  set_org_evt;
};

static struct test_data ttm;
static char ap_buf[128];
static char fb_buf[128];
static char evt_buf[128];
static char org_evt_buf[128];

struct test_conf test_cfg[] = {
    {0, PNL0_FB_NUM, PNL0_DEFAULT_AP, PNL0_DEFAULT_EVT, PNL0_ORG_EVT},
    {1, PNL1_FB_NUM, PNL1_DEFAULT_AP, PNL1_DEFAULT_EVT, PNL1_ORG_EVT},
    {2, PNL2_FB_NUM, PNL2_DEFAULT_AP, PNL2_DEFAULT_EVT, PNL2_ORG_EVT}
};

#define MAX_TEST_CFG   (Q_ELEMENTS(test_cfg))
#define OPT_ORG_EVT (1)
#define use_arg(flag, max, arg, idx) ( flag && idx < max && arg[idx] >= 0 )

struct option long_opts[] = {
    {"master",              1, 0,   'p'},
    {"pnlnum",              1, 0,   'n'},
    {"event",               1, 0,   'e'},
    {"fb",                  1, 0,   'f'},
    {"ap",                  1, 0,   'a'},
    {"input",               1, 0,   OPT_ORG_EVT},
    {"split",               0, 0,   's'},
    {"calibrate",           0, 0,   'c'},
    {"help",                0, 0,   'h'},
    {"version",             0, 0,   'v'},
    {0,                     0, 0,   0}
};

struct pos_mode mono_mode[] = {
    {0,   0, 100, 100}
};
struct pos_mode de_mode[] = {
    {0,   0,  50, 100},
    {50,  0,  50, 100}
};
struct pos_mode tri_mode[] = {
    {0,   0,  33, 100},
    {33,  0,  34, 100},
    {67,  0,  33, 100}
};
struct pos_mode tetra_mode[] = {
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
        char *target;
}g_ipc;

typedef struct _cmd_general {
    char hdr;
    char raw_data[15];
}cmd_general_t;

typedef struct _cmd_append {
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

typedef struct _cmd_clear {
    char hdr;
    char panel;
} cmd_clear_t;

typedef struct _tm_cmd {
    char len;
    union{
        unsigned char   data[16];
        cmd_general_t   general;
        cmd_append_t    append;
        cmd_clear_t     clear;
    };
} tm_cmd_t;

void tm_test_usage()
{
    fprintf(stderr, "Usage: tm-test OPTION\n"
                    "OPTION\n"
                    "   -p  --master        panel which is set\n"
                    "   -a  --ap            append application programs\n"
                    "   -e  --event         append touch event device\n"
                    "       --input         set touch event device for calibration\n"
                    "   -f  --fb            append frame buffer device\n"
                    "   -n  --pnlnum        set panel number\n"
                    "   -s  --split         split mode\n"
                    "   -c  --calibrate     calibrateon\n"
                    "   -h  --help          show usage\n"
                    "   -v  --version       show version\n");
    _exit(0);
}

void tm_test_version()
{
    fprintf(stderr,"tm-test version : %s\n",TM_TEST_VERSION);
    _exit(0);
}

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
//    printf("recv len %d, from %s\n", len, from);
//
//    for (int i = 0; i < len; i++) {
//        printf("0x%2x ",msg[i]);
//    }
//    printf("\n");

    if (msg[0] == 0xd0)
    {
        ttm.wait_ver=0;
    }
}
int open_ipc()
{
    int retry;

    g_ipc.status = PROTOCOL_IDLE;
    g_ipc.recv_func = recv_event;
    g_ipc.name = TEST_IPC_NAME;
    g_ipc.target = TEST_IPC_TARGET;

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

   g_ipc.status=qsi_send_buffer(g_ipc.server, g_ipc.target, cmd->data, cmd->len, 0); 

   if(g_ipc.status!=PROTOCOL_ACK_OK)
       printf("send ipc error\n");
}

int get_fb(int pnl)
{
    if(use_arg(ttm.set_fb, ttm.fb_num, ttm.fb_arg, pnl))
        return ttm.fb_arg[pnl];
    else
        return (pnl<MAX_TEST_CFG) ? test_cfg[pnl].fb : -1;
}
int get_ap(int pnl)
{
    if(use_arg(ttm.set_ap, ttm.ap_num, ttm.ap_arg, pnl))
        return ttm.ap_arg[pnl];
    else
        return (pnl<MAX_TEST_CFG) ? test_cfg[pnl].ap : -1;
}
int get_evt(int pnl)
{
    if(use_arg(ttm.set_evt, ttm.evt_num, ttm.evt_arg, pnl))
        return ttm.evt_arg[pnl];
    else
        return (pnl<MAX_TEST_CFG) ? test_cfg[pnl].evt : -1;
}
int get_org_evt(int pnl)
{
    if(use_arg(ttm.set_org_evt, ttm.org_evt_num, ttm.org_evt_arg, pnl))
        return ttm.org_evt_arg[pnl];
    else
        return (pnl<MAX_TEST_CFG) ? test_cfg[pnl].org_evt : -1;
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
    int i,j,max_pnl=PNL_NUM;

    if(ttm.set_pnl_num)
        max_pnl = ttm.pnl_num_arg;

    for(i=0; i<max_pnl; i++)
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
    evt->num = get_ap(pnl);
    evt->dev[EVT_NUM_POS] = '0' + evt->num;
}

void set_ttm()
{
    int i, max_pnl=PNL_NUM;

    if(ttm.set_pnl_num)
        max_pnl = ttm.pnl_num_arg;

    for(i=0; i<max_pnl; i++)
    {
        init_evt(&ttm.evt[i], i);
        init_fb(&ttm.fb[i], i);
    }

    for(i=0; i<ttm.ap_num; i++)
    {
        int pnl = select_pnl(ttm.ap_arg[i]);
        ttm.evt[pnl].act = 1;

        // if panel supports multi-ap, replace default ap for ts_test
        switch(pnl)
        {   
            case 0:
                ttm.evt[0].num = ttm.ap_arg[i];
                ttm.evt[0].dev[EVT_NUM_POS] = '0' + ttm.evt[0].num;
                break;
            case 1:
            case 2:
            default:
                break;
        }
    }
}

void set_pnl_append_cmd(cmd_append_t* a, int idx)
{
    struct pos_mode (*pconf)[] = NULL;

    if(idx > ttm.mode)
        return;
        
    switch(ttm.mode)
    {
        case MONO_AP:   pconf = &mono_mode;     break;
        case DE_AP:     pconf = &de_mode;       break;
        case TRI_AP:    pconf = &tri_mode;      break;
        case TETRA_AP:  pconf = &tetra_mode;    break;
        case PENTA_AP:
        case HEXA_AP:
        case HEPTA_AP:
        case OCTA_AP:
        case NONA_AP:
        case DECA_AP:
        default:        break;
    }
    
    if(!pconf)
        return;

    a->pnl_start_pos_x = (*pconf)[idx].pos_x;
    a->pnl_start_pos_y = (*pconf)[idx].pos_y;
    a->pnl_width       = (*pconf)[idx].width;
    a->pnl_high        = (*pconf)[idx].high;           
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
    
    struct pos_mode (*mode)[] = NULL;

    if(idx > ttm.mode)
        return;
        
    switch(ttm.mode)
    {
        case MONO_AP:   mode = &mono_mode;     break;
        case DE_AP:     mode = &de_mode;       break;
        case TRI_AP:    mode = &tri_mode;      break;
        case TETRA_AP:  mode = &tetra_mode;    break;
        case PENTA_AP:
        case HEXA_AP:
        case HEPTA_AP:;
        case OCTA_AP:
        case NONA_AP:
        case DECA_AP:
        default:        break;
    }
    
    if(!mode)
        return;

    a->ap_start_pos_x = (*mode)[idx].pos_x;
    a->ap_start_pos_y = (*mode)[idx].pos_y;
    a->ap_width       = (*mode)[idx].width;
    a->ap_high        = (*mode)[idx].high;   
}

void tm_calibrate()
{
    int fd, i, max_pnl=PNL_NUM;
    fb_data_t fb;
    evt_data_t evt;
    
    if(ttm.set_pnl_num)
        max_pnl = ttm.pnl_num_arg;

    memset(&fb, 0, sizeof(fb_data_t));
    memset(&evt, 0, sizeof(evt_data_t));

    memcpy(fb.dev, default_fb, sizeof(default_fb));
    memcpy(fb.pan, default_pan, sizeof(default_pan));
    memcpy(evt.dev, default_evt, sizeof(default_evt));

    if(system("mount -o remount -w /"))
        fprintf(stderr, "file system may be readonly!!\n");

    for (i = 0; i < max_pnl; i++) 
    {
        fb.fb_id = get_fb(i);
        fb.dev[FB_NUM_POS]= '0' + fb.fb_id;
        fb.pan[PAN_NUM_POS]= '0' + fb.fb_id;
        evt.num = get_org_evt(i);

        if((evt.dev[EVT_NUM_POS] = evt.num/10))
        {
            evt.dev[EVT_NUM_POS] += '0';
            evt.dev[EVT_NUM_POS+1] = '0' + evt.num%10;
        }
        else
            evt.dev[EVT_NUM_POS] = '0' + evt.num;
    
        printf("calibrate %d pan : %s\n",i,fb.pan);
        printf("calibrate %d fb  : %s\n",i,fb.dev);
        printf("calibrate %d evt : %s\n",i,evt.dev);
        
        if((fd=open(fb.pan, O_RDWR))>=0)
        {
            if (write(fd, "0,0", 3)<0) {
                dbg_log("write pan error, pan : %s",fb.pan);
            }
            close(fd);
        }
        else
        {
            printf("open pan error\n");
        }
        ts_cal(&fb, evt.dev);
    }

    update_calibrate();
    refresh_tm_test(&fb);

    system("mount -o remount -r /");
}

int set_args(char* buf, int* args, int max)
{
    int i=0;
    char *param;
    
    param = strtok(buf, ",");
    args[i++] = atoi(param);

    while( (param = strtok(NULL, ",")) && i < max)
    {
        args[i++] = atoi(param);
    }

    return i;
}

void parse_options()
{
    if(ttm.set_ap)
        ttm.ap_num = set_args(ap_buf, ttm.ap_arg, MAX_AP_NUM);

    if(ttm.set_fb)
        ttm.fb_num = set_args(fb_buf, ttm.fb_arg, MAX_PNL_NUM);

    if(ttm.set_evt)
        ttm.evt_num = set_args(evt_buf, ttm.evt_arg, MAX_PNL_NUM);
    
    if(ttm.set_org_evt)
        ttm.org_evt_num = set_args(org_evt_buf, ttm.org_evt_arg, MAX_PNL_NUM);
}

void show_args_for_debug()
{
    int i, max_pnl;

    if(ttm.set_pnl_num)
        max_pnl = ttm.pnl_num_arg;
    else
        max_pnl = PNL_NUM; 

    for(i=0;i<max_pnl;i++)
    {
        printf("%d -> ap  : %2d, ",i, get_ap(i));
        printf("evt : %2d, ",get_evt(i));
        printf("fb  : %2d, ",get_fb(i));
        printf("org : %2d\n",get_org_evt(i));
    }

    printf("panel     : %d\n",ttm.pnl_arg);
    printf("calibrate : %d\n",ttm.calibrate);
    printf("split     : %d\n",ttm.split);
}


int main(int argc, const char *argv[])
{ 
    tm_cmd_t cmd;
    char i;
    int fd, opt_idx, c, max_pnl=PNL_NUM;
    char *short_opts = "p:sa:n:e:f:chv";
    pid_t pid;

    memset((char*)&cmd, 0, sizeof(cmd));

    while ((c = getopt_long(argc, (char* const*)argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'a':
                ttm.set_ap = 1;
                memcpy(ap_buf, optarg, strlen(optarg));
                break;
            case 'n':
                ttm.set_pnl_num = 1;
                ttm.pnl_num_arg = optarg[0] - '0';
                break;
            case 'e':
                ttm.set_evt = 1;
                memcpy(evt_buf, optarg, strlen(optarg));
                break;
            case 'f':
                ttm.set_fb = 1;
                memcpy(fb_buf, optarg, strlen(optarg));
                break;
            case OPT_ORG_EVT:
                ttm.set_org_evt = 1;
                memcpy(org_evt_buf, optarg, strlen(optarg));
                break;
            case 'c':
                ttm.calibrate = 1;
                break;
            case 'p':
                ttm.pnl_arg = optarg[0] - '0';
                break;
            case 's':
                ttm.split = 1;
                break;
            case 'h':
                tm_test_usage();
                break;
            case 'v':
                tm_test_version();
                break;
            default:
                break;
        }
    }

    parse_options();

#if TEST_DEBUG
    show_args_for_debug();
#endif

    if(ttm.calibrate)
    {
        tm_calibrate();
        return 0;
    }

    if(ttm.set_pnl_num)
        max_pnl = ttm.pnl_num_arg;
    
    if(ttm.pnl_arg < 0 || ttm.pnl_arg > max_pnl)
    {
        printf("set panel error\n");
        tm_test_usage();
    }    

    if(!ttm.set_ap)
    {
        printf("no set ap\n");
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
    for (i = 0; i < max_pnl; i++) 
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

//    cmd.general.hdr=0xd0;
//    cmd.len=1;
//    ttm.wait_ver=1;
//    send_ipc(&cmd);
//
//    while(ttm.wait_ver)
//        sleep(1);

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
