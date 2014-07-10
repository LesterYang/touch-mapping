/*
 * qInput.h
 *
 *  Created on: Jul 9, 2014
 *      Author: lester
 */

#ifndef QINPUT_H_
#define QINPUT_H_

#define MAXEVDEVS (8)
#define EVDEVNAME "/dev/input/event%d"


int  tm_inputInit();
void tm_inputDeinit();

#endif /* QINPUT_H_ */
