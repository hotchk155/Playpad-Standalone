#ifndef _PLAYPAD_H_
#define _PLAYPAD_H_

#include "vos.h"


#include "USBHost.h"
#include "ioctl.h"
#include "UART.h"
#include "GPIO.h"
#include "string.h"
#include "errno.h"
#include "timers.h"

typedef unsigned char byte;

#define VOS_DEV_GPIO_A		   		0
#define VOS_DEV_UART		   		1
#define VOS_DEV_USBHOST_1	  		2
#define VOS_DEV_USBHOST_2	   		3
#define VOS_DEV_USBHOSTGENERIC_1   	4
#define VOS_DEV_USBHOSTGENERIC_2   	5
#define VOS_DEV_TIMER0   			6
#define VOS_NUMBER_DEVICES	   		7

#define LED_RED_OFF 	0x00
#define LED_RED_LOW 	0x01
#define LED_RED_MED 	0x02
#define LED_RED_HI  	0x03

#define LED_GREEN_OFF 	0x00
#define LED_GREEN_LOW 	0x10
#define LED_GREEN_MED 	0x20
#define LED_GREEN_HI  	0x30

#define LED_COPY		0x04
#define LED_CLEAR		0x08
	

#define LED_OFF			0x00
#define LED_RED			LED_RED_HI
#define LED_GREEN		LED_GREEN_HI
#define LED_YELLOW		(LED_RED_HI|LED_GREEN_HI)
	
enum {
	EVENT_KEYDOWN	 = 1,
	EVENT_KEYUP,	 
	EVENT_TICK
};
	
extern void LPSetLED(int row, int col, byte colour);				
extern void MIDIPlayNote(byte note, byte velocity);
extern void MIDIStopNote(byte note);
extern void LPBeginBufferedUpdate();
extern void LPEndBufferedUpdate();

extern void echo_init();
extern void echo_done();
extern void echo_event(int type, int row, int col);

extern void rainstorm_init();
extern void rainstorm_done();
extern void rainstorm_event(int type, int row, int col);

extern void arpie_init();
extern void arpie_done();
extern void arpie_event(int type, int row, int col);

#endif // _PLAYPAD_H_

