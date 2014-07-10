/*
 * qInput.c
 *
 *  Created on: Jul 9, 2014
 *      Author: lester
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <linux/input.h>

#include "qInput.h"

typedef struct sPhoneDat {
    int sk, nsk;
    fd_set skfds;
    fd_set  evfds;
    int maxfd,evdev;

    qsi_thread  *thread;

    struct {
        volatile char initDone      :1;
        volatile char getPanelEvent :1;
        volatile char keyBarWait    :1;
        volatile char keyBar        :1;
        volatile char openInput     :1;
        volatile char iUImutexLock  :1;
        char reserved               :2;
    }flag;
}sPhoneDat;

sPhoneDat vsPhoneDat;
int g_evDev[MAXEVDEVS];
volatile int g_absX,g_absY;

void F_PhoneInitInput();
void F_PhoneDeinitInput();
int  F_PhoneInitEvents();
void F_PhoneCloseEvents();
void F_PhoneCleanupStdin();
int  F_PhoneAddFileDescriptors (fd_set * fdsp);
int  F_PhoneParseDev();
static void F_PhoneThreadInput(void *data);

void F_PhoneInitInput()
{
    if ( 1 > F_PhoneInitEvents()){
        fprintf ( stderr, "Failed to open event interface files\n" );
    }

    vsPhoneDat.maxfd = F_PhoneAddFileDescriptors( &vsPhoneDat.evfds );

    if ( vsPhoneDat.maxfd <= 0 ){
        fprintf ( stderr, "Failed to organize event input.\n" );
    }

    vsPhoneDat.thread = qsi_thread_new(F_PhoneThreadInput, NULL);
}

void F_PhoneDeinitInput()
{
    F_PhoneCloseEvents();

    usleep(50);

    if (vsPhoneDat.thread){
        qsi_thread_delet(vsPhoneDat.thread);
        vsPhoneDat.thread = NULL;
    }
}

//only one device, if number vsPhoneDat.evdev is >= 0
int F_PhoneInitEvents()
{
    int  idx, event;
    char buf[sizeof(EVDEVNAME)+8];

    for (idx=0; idx<MAXEVDEVS; ++idx){
        g_evDev[idx] = -1;
    }

    for (idx=event=0; event<MAXEVDEVS; ++event){
        if ((vsPhoneDat.evdev >= 0) && (vsPhoneDat.evdev != event))
            continue;

        sprintf (buf, EVDEVNAME, event);
        g_evDev[idx] = open (buf, O_RDONLY );
        if (0 <= g_evDev[idx]){
            fprintf (stdout, "Opened %s as event device [counter %d]\n",buf, idx);
            ++idx;
        }
    }

    if (idx > 0)
        vsPhoneDat.flag.openInput = TRUE;

    g_absX = g_absY = 0;
    return idx;
}

void F_PhoneCloseEvents()
{
    int idx;
    for (idx=0; idx<MAXEVDEVS; ++idx){
        if (g_evDev[idx] >= 0){
            qsi_close(g_evDev[idx]);
            fprintf (stdout, "Closed event device [counter %d]\n", idx);
        }
    }
    vsPhoneDat.flag.openInput = FALSE;
    return;
}

void F_PhoneCleanupStdin()
{
    fd_set fds;
    struct timeval tv;
    char buf[8];
    int stdin = 0, stdout = 1;

    FD_ZERO (&fds);
    FD_SET (stdin, &fds);
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    while (0 < select(stdout, &fds, NULL, NULL, &tv)){
        while (qsi_read(stdin, buf, 8, 0)) {;}
        FD_ZERO (&fds);
        FD_SET (stdin, &fds);
        tv.tv_sec  = 0;
        tv.tv_usec = 1;
    }
    qsi_close(stdin);
    return;
}

int F_PhoneAddFileDescriptors(fd_set * fdsp)
{
    int idx, maxfd;
    FD_ZERO (fdsp);

    maxfd = -1;

    for (idx=0; idx<MAXEVDEVS; ++idx)
    {
        if (g_evDev[idx] >= 0)
        {
            FD_SET (g_evDev[idx], fdsp);

            if (g_evDev[idx] > maxfd)
                maxfd = g_evDev[idx];
        }
    }
    return maxfd;
}

int F_PhoneParseDev()
{
    int i, j, qtx, qty;
    char buf[sizeof(struct input_event)] = {0};
    struct input_event *inEvent = (struct input_event *)buf;

    if (&vsPhoneDat.evfds == NULL)
        return -1;

    for (i=0; i<MAXEVDEVS; ++i){

        if (0 > g_evDev[i])
            continue;

        if (!(FD_ISSET(g_evDev[i], &vsPhoneDat.evfds)))
            continue;

        j = read (g_evDev[i], buf, sizeof(struct input_event));
        if ( (j == 0) || (j == -1) || ((int)sizeof(struct input_event) > j) )
            continue;
    }

    if (vsPhoneDat.flag.getPanelEvent == FALSE)
        return 0;

    switch (inEvent->type){
        case EV_SYN:
            switch (g_inputFlag){
                case ePhoneInputTypeRelease:
                    F_PhoneTimerChange(ePhoneTimerStart);
                    break;

                case ePhoneInputTypeTouch:
                    g_inputFlag = ePhoneInputTypeEnter;
                    break;

                case ePhoneInputTypeEnter:
                    if (g_absX == 0 || g_absY == 0){
                        return 0;
                    }

                    if (g_timerFlag == ePhoneTimerCounting){
                        F_PhoneTimerChange(ePhoneTimerStop);
                        F_PhoneHidPannelTouch((short)g_absX, (short)g_absY, ePhoneHidTouchTypeDrag);
                    }
                    else{
                        F_PhoneIsKey(ePhoneHidKeyTypePress);
                        F_PhoneHidPannelTouch((short)g_absX, (short)g_absY, ePhoneHidTouchTypeEnter);
                    }
                    g_inputFlag = ePhoneInputTypeDrag;

                    break;

                case  ePhoneInputTypeDrag:
                    if (g_absX == 0 || g_absY == 0){
                        return 0;
                    }

                    F_PhoneHidPannelTouch((short)g_absX, (short)g_absY, ePhoneHidTouchTypeDrag);

                    if (vsPhoneDat.flag.keyBarWait)
                        F_PhoneIsKey(ePhoneHidKeyTypeDrag);

                    break;

                case ePhoneInputTypeNone:
                default:
                    break;
            }
            break;
        case EV_KEY:
            switch (inEvent->code){
                case BTN_TOUCH:
                        if((inEvent->value)){
                            g_inputFlag = ePhoneInputTypeTouch;
                        }
                        else{
                            switch(g_inputFlag){
                                case ePhoneInputTypeEnter:
                                    fprintf(stdout, "abandon Enter !!!\n");
                                    g_inputFlag = ePhoneInputTypeNone;
                                    break;
                                default:
                                    g_inputFlag = ePhoneInputTypeRelease;
                                    break;
                            }
                        }
                    break;
                default:
                    break;
            }
            break;

        case EV_ABS:

            switch (inEvent->code){

#if(PANEL_TYPE==PANEL_CDR019)
                case ABS_Y:
                    if(!(inEvent->value <= MAX_PN_X && inEvent->value >= MIN_PN_X))
                        return 0;
                    qtx = (QT_X * (inEvent->value - MIN_PN_X)) / LEN_PN_X;
                    g_deltaX =  qtx - g_absX;
                    g_absX = qtx;
                    break;

                case ABS_X:
                    if(!(inEvent->value >= MIN_PN_Y && inEvent->value <= MAX_PN_Y))
                        return 0;
                    qty = (QT_Y * (inEvent->value - MIN_PN_Y)) / LEN_PN_Y;
                    g_deltaY = qty - g_absY;
                    g_absY = qty;
                    break;

                case ABS_PRESSURE:
                    break;

#elif(PANEL_TYPE==PANEL_AVN001)
                case ABS_X:
                    if(!(inEvent->value <= MAX_PN_X && inEvent->value >= MIN_PN_X))
                        return 0;
                    qtx = QT_X - ((QT_X * (inEvent->value - MIN_PN_X)) / LEN_PN_X);
                    g_absX = qtx;
                    break;

                case ABS_Y:
                    if(!(inEvent->value >= MIN_PN_Y && inEvent->value <= MAX_PN_Y))
                        return 0;
                    qty = (QT_Y * (inEvent->value - MIN_PN_Y)) / LEN_PN_Y;
                    g_absY = qty;
                    break;

                case ABS_PRESSURE:
                    break;
#endif
            }
        default:
            break;
    }
    return 0;
}

static void F_PhoneThreadInput(void *data)
{
    struct timeval tv;

    while (vsPhoneDat.flag.openInput){

        F_PhoneAddFileDescriptors ( &vsPhoneDat.evfds );
        tv.tv_sec  = 0;
        tv.tv_usec = 500000;

        while ( 0 < select((vsPhoneDat.maxfd)+1, &vsPhoneDat.evfds, NULL, NULL, &tv) ){

            if ( 0 > F_PhoneParseDev() ){
                fprintf(stderr, "IO thread failed\n");
                break;
            }

            if(vsPhoneDat.flag.openInput == FALSE)
                break;

            F_PhoneAddFileDescriptors(&vsPhoneDat.evfds);
            tv.tv_sec  = 0;
            tv.tv_usec = 500000;
        }
    }
}
