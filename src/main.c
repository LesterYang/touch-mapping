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
#include <sys/stat.h>
#include "tm.h"
#include "tmIpc.h"

#define TM_MAIN_DELAY (5)

struct tm_status_info{
    tm_status_t status;
    const char* str;
};

struct tm_status_info status_info[] = {
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
    {"daemon",              0, 0,   'd'},
    {0,                     0, 0,   0}
};

tm_status_t g_status = TM_STATUS_NONE;
q_bool g_daemonise = q_false;

void tm_usage(char* arg)
{
    fprintf(stderr, "Usage : %s [OPTIONS]\n"
                    "OPTIONS\n"
                    "   -d, --daemon    daemonise\n"
                    "   -h, --help      help\n"
                    "   -v, --version   show version\n"
                    ,arg);
    exit(0);
}

void tm_version()
{
    q_dbg(Q_INFO,"version %s", TM_VERSION);
    exit(0);
}

void tm_shutdown(int signum)
{
    q_dbg(Q_INFO,"shutdown!!");
    tm_deinit();
    tm_close_ipc();
    exit(signum);
}

void tm_switch_main_status(tm_status_t status)
{
     q_dbg(Q_INFO,"status, %10s -> %s", q_strnull(status_info[g_status].str), q_strnull(status_info[status].str));
     g_status = status;
}

void tm_daemonise()
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
    int opt_idx;
    char *short_opts = "hdv";
    int c;

    while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_idx)) != -1)
    {
        switch(c)
        {
            case 'd':
                g_daemonise = q_true;
                break;
            case 'h':
                tm_usage(argv[0]);
                break;
            case 'v':
                tm_version();
                break;
            default:
                break;
        }
    }

    if (g_daemonise)
        tm_daemonise();

    while(g_status != TM_STATUS_EXIT)
    {
        switch(g_status)
        {
            case TM_STATUS_NONE:
                q_dbg(Q_INFO,"version %s", TM_VERSION);
                tm_switch_main_status(TM_STATUS_INIT);
                break;

            case TM_STATUS_INIT:
                if(tm_init() == TM_ERRNO_SUCCESS)
                {
                    signal(SIGINT, tm_shutdown);
                    tm_bind_status(&g_status);
                    tm_switch_main_status(TM_STATUS_IPC_INIT);
                }
                else
                   sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_IPC_INIT:
            	if(!tm_open_ipc())
            	    tm_switch_main_status(TM_STATUS_RUNNING);
            	else
            	    sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_RUNNING:
                sleep(TM_MAIN_DELAY);
                break;

            case TM_STATUS_DEINIT:
                tm_deinit();
                tm_close_ipc();
                tm_switch_main_status(TM_STATUS_EXIT);
                break;

            case TM_STATUS_REINIT:

		tm_switch_main_status(TM_STATUS_INIT);
		break;

            case TM_STATUS_ERROR:
                tm_switch_main_status(TM_STATUS_DEINIT);
                break;

            case TM_STATUS_EXIT:
                break;

            default:
                tm_switch_main_status(TM_STATUS_ERROR);
                break;
        }
    }
    return 0;
}

