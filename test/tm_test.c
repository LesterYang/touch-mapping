#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include "tm_test.h"
#include <qsi_ipc_client_lib.h>

typedef struct _fb_data_t {
    int   num;
    char* dev;
    char* pan;
} fb_data_t;

typedef struct _evt_data_t {
    int   num;
    char* dev;
} evt_data_t;


// depend on panel[0-2] <-> kernel setting
fb_data_t fb_data[]={
    {0, "/dev/fb0", "/sys/class/graphics/fb0/pan"},
    {4, "/dev/fb4", "/sys/class/graphics/fb4/pan"},
    {9, "/dev/fb9", "/sys/class/graphics/fb9/pan"}
};
// depend on panel[0-2] <-> ap setting
evt_data_t evt_data[]={
    {0, "/dev/input/event0"},
    {4, "/dev/input/event4"},
    {6, "/dev/input/event6"}
};


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
        char            data[16];
        cmd_append_t    append;
        cmd_clear_t     clear;
    };
} tm_cmd_t;



thread_data_t d[3];

/*
static void sig(int sig)
{
	close_framebuffer(&d[0]);
        close_framebuffer(&d[1]);
        close_framebuffer(&d[2]);
        
        fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	_exit(1);
}
*/

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

void test_thread_func(void* data)
{
    thread_data_t* d=(thread_data_t*)data; 
    ts_test(d);
}

void send_ipc(tm_cmd_t* cmd)
{       
  int i;
  printf("send ipc ");
  for (i = 0; i < cmd->len; i++) {
      printf("%d ",cmd->data[i]);
  }
  printf("\n");

   g_ipc.status=qsi_send_buffer(g_ipc.server, "QSIPL3", cmd->data, cmd->len, 0); 

   if(g_ipc.status!=PROTOCOL_ACK_OK)
       printf("send ipc error\n");
}

int main(int argc, const char *argv[])
{ 
    tm_cmd_t cmd;
    fb_data_t* fb;
    evt_data_t* evt;
    char pnl=0x1F,ap=0x1F,i;
    int opt_idx;
    char *short_opts = "p:s:a:";
    int c;

    memset((char*)&cmd, 0, sizeof(cmd));

    while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'a':
                ap = optarg[0] - '0';
                break;
            case 'p':
                pnl = optarg[0] - '0';
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
        send_ipc(&cmd);
    }

    // set panel fb,event
    fb  = &fb_data[pnl];
    evt = &evt_data[pnl];

    if(pnl < 3)
    {
        int fd;
        char* arg="0,0";

        fb=&fb_data[pnl];
        
        if((fd=open(fb->pan, O_RDWR))<0)
            printf("open pan error\n");

        printf("write %s to %s\n",arg,fb->pan);

        if (write(fd, arg, sizeof(arg))<0)  
            printf("write pan error\n");

        close(fd);
    }

    char ts_test_cmd[64]={0};
    char* ts_test_script="./tm_test.sh";
    char* space_str=" ";
    int done=0;


    memcpy(&ts_test_cmd[done], ts_test_script, strlen(ts_test_script));
    done += strlen(ts_test_script);

    memcpy(&ts_test_cmd[done], " ", 1);
    done++;

    memcpy(&ts_test_cmd[done], fb->dev, strlen(fb->dev));
    done += strlen(fb->dev);

    memcpy(&ts_test_cmd[done], " ", 1);
    done++;

    memcpy(&ts_test_cmd[done], evt->dev, strlen(evt->dev));
    done += strlen(evt->dev);

    ts_test_cmd[done]=0x0;

    system(ts_test_cmd); 

    //ts_test();


    if(g_ipc.server)
    {
        qsi_close_channel(g_ipc.server);
        g_ipc.server = NULL;
    }


//
//    d[0].ap=0;
//    d[0].fb="/dev/fb0";
//    d[0].event="/dev/input/event0";
//  
//    d[1].ap=1;
//    d[1].fb="/dev/fb1";
//    d[1].event="/dev/input/event1";
//  
//    d[2].ap=2;
//    d[2].fb="/dev/fb2";
//    d[2].event="/dev/input/event2";
//
//    signal(SIGSEGV, sig);
//    signal(SIGINT, sig);
//    signal(SIGTERM, sig);
//
//    pthread_create(&d[0].id, NULL, test_thread_func, &d[0]);
//    pthread_create(&d[1].id, NULL, test_thread_func, &d[1]);
//    pthread_create(&d[2].id, NULL, test_thread_func, &d[2]);
//
//    pthread_join(d[0].id, NULL);
//    pthread_join(d[1].id, NULL);
//    pthread_join(d[2].id, NULL);
//    
    return 0;
}
