#include "Playpad.h"
	
void echo_init()
{
}

void echo_done()
{
}

void echo_event(int type, int row, int col)
{
	if(type == EVENT_KEYDOWN)
	{
		LPSetLED(row, col, LED_YELLOW);				
	}
	else if(type == EVENT_KEYUP)
	{
		LPSetLED(row, col, 0);				
	}
}
	
