#include <stdio.h>
#include <unistd.h>
#include "tm_test.h"
#include <qsi_ipc_client_lib.h>

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


thread_data_t d[3];


static void sig(int sig)
{
	close_framebuffer(&d[0]);
        close_framebuffer(&d[1]);
        close_framebuffer(&d[2]);
        
        fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
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
    g_ipc.name = "QSIQT3";

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

int main(int argc, const char *argv[])
{ 
    open_ipc();

    d[0].ap=0;
    d[0].fb="/dev/fb0";
    d[0].event="/dev/input/event0";
  
    d[1].ap=1;
    d[1].fb="/dev/fb1";
    d[1].event="/dev/input/event1";
  
    d[2].ap=2;
    d[2].fb="/dev/fb2";
    d[2].event="/dev/input/event2";

    signal(SIGSEGV, sig);
    signal(SIGINT, sig);
    signal(SIGTERM, sig);




    pthread_create(&d[0].id, NULL, test_thread_func, &d[0]);
    pthread_create(&d[1].id, NULL, test_thread_func, &d[1]);
    pthread_create(&d[2].id, NULL, test_thread_func, &d[2]);

    pthread_join(d[0].id, NULL);
    pthread_join(d[1].id, NULL);
    pthread_join(d[2].id, NULL);
    
    return 0;
}
