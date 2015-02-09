/*
 *  main.c
 *  Copyright © 2014 QSI Inc.
 *  All rights reserved.
 *  
 *       Author : Lester Yang <lester.yang@qsitw.com>
 *  Description : tm-daemon 
 */
 
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include "tm.h"
#include "tmIpc.h"

#define TM_MAIN_DELAY (5)

typedef enum _tm_status     tm_status_t;

enum _tm_status{
    TM_STATUS_NONE,
    TM_STATUS_INIT,
    TM_STATUS_IPC_INIT,
    TM_STATUS_RUNNING,
    TM_STATUS_DEINIT,
    TM_STATUS_REINIT,
    TM_STATUS_ERROR,
    TM_STATUS_EXIT
};

struct status_info{
    tm_status_t status;
    const char* str;
};

struct status_info status_info[] = {
    {TM_STATUS_NONE,        "None"},
    {TM_STATUS_INIT,        "Init"},
    {TM_STATUS_IPC_INIT,    "IPCInit"},
    {TM_STATUS_RUNNING,     "Running"},
    {TM_STATUS_DEINIT,      "Deinit"},
    {TM_STATUS_REINIT,      "Reinit"},
    {TM_STATUS_ERROR,       "Error"},
    {TM_STATUS_EXIT,        "Exit"},
    {-1,                    NULL},
};

struct option long_opts[] = {
    {"help",                0, 0,   'h'},
    {"version",             0, 0,   'v'},
    {"client",              0, 0,   'c'},
    {"daemon",              0, 0,   'd'},
    {0,                     0, 0,   0}
};

tm_status_t g_status = TM_STATUS_NONE;
q_bool g_daemonise = q_false;
char g_name[IPC_MAX_NAME]={0};

void usage(char* arg)
{
    fprintf(stderr, "Usage : %s [OPTIONS]\n"
                    "OPTIONS\n"
                    "   -c, --client    IPC client\n"
                    "   -d, --daemon    daemonise\n"
                    "   -h, --help      help\n"
                    "   -v, --version   show version\n"
                    ,arg);
    exit(0);
}

void version()
{
    q_dbg(Q_INFO,"version %s", TM_VERSION);
    exit(0);
}

void shutdown(int signum)
{
    q_dbg(Q_INFO,"shutdown!!");
    tm_deinit();
    tm_ipc_close();
    exit(signum);
}

void switch_main_status(tm_status_t status)
{
     q_dbg(Q_INFO,"status, %10s -> %s", q_strnull(status_info[g_status].str), q_strnull(status_info[status].str));
     g_status = status;
}

void daemonise()
{
    pid_t pid = fork();
    if (pid == -1) {
        q_dbg(Q_ERR,"failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        _exit(0);
    }

    // Start a new session for the daemon.
    if (setsid()==-1) {
        q_dbg(Q_ERR,"failed to become a session leader while daemonising(errno=%d)",errno);
    }

    // Fork again, allowing the parent process to terminate.
    signal(SIGHUP,SIG_IGN);

    pid=fork();
    if (pid == -1) {
        q_dbg(Q_ERR,"failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        _exit(0);
    }

    // Set the current working directory to the root directory.
    if (chdir("/") == -1) {
        q_dbg(Q_ERR,"failed to change working directory while daemonising (errno=%d)",errno);
    }

    // Set the user file creation mask to zero.
    umask(0);

    // Close then reopen standard file descriptors.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
        q_dbg(Q_ERR,"failed to reopen stdin while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_WRONLY) == -1) {
        q_dbg(Q_ERR,"failed to reopen stdout while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_RDWR) == -1) {
        q_dbg(Q_ERR,"failed to reopen stderr while daemonising (errno=%d)",errno);
    }
}

int main(int argc, char* argv[])
{
    int opt_idx, len;
    char *short_opts = "hdvc:";
    int c;

    while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'c':
                if((len = strlen(optarg)) < IPC_MAX_NAME)
                    memcpy(g_name, optarg, len);
                break;

            case 'd':
                g_daemonise = q_true;
                break;

            case 'h':
                usage(argv[0]);
                break;

            case 'v':
                version();
                break;

            default:
                break;
        }
    }

    if (g_daemonise)
        daemonise();

    while(g_status != TM_STATUS_EXIT)
    {
        switch(g_status)
        {
            case TM_STATUS_NONE:
                q_dbg(Q_INFO,"version %s", TM_VERSION);
                switch_main_status(TM_STATUS_INIT);
                break;

            case TM_STATUS_INIT:
                if(tm_init() == TM_ERRNO_SUCCESS)
                {
                    signal(SIGINT, shutdown);
                    switch_main_status(TM_STATUS_IPC_INIT);
                }
                else
                   sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_IPC_INIT:
            	if(!tm_ipc_open(g_name))
            	    switch_main_status(TM_STATUS_RUNNING);
            	else
            	    sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_RUNNING:
                sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_DEINIT:
                tm_deinit();
                tm_ipc_close();
                switch_main_status(TM_STATUS_EXIT);
                break;

            case TM_STATUS_REINIT:
                switch_main_status(TM_STATUS_INIT);
                break;

            case TM_STATUS_ERROR:
                switch_main_status(TM_STATUS_DEINIT);
                break;

            case TM_STATUS_EXIT:
                break;

            default:
                switch_main_status(TM_STATUS_ERROR);
                break;
        }
    }
    return 0;
}

