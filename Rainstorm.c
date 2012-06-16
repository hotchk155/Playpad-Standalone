#include "stdlib.h"
#include "string.h" //memset
#include "Playpad.h"

	
typedef struct {
	byte cycle[8];
	byte note[8];
	byte state[8][8];
	byte divisor;
	unsigned long ticks;
	int blocks;
} RAINSTORM_DATA;
	
	
RAINSTORM_DATA *pRS = NULL;	
void rainstorm_init()
{
	int col;
	pRS = (RAINSTORM_DATA*)malloc(sizeof(RAINSTORM_DATA));
	memset(pRS,0,sizeof(RAINSTORM_DATA));
	pRS->ticks = 0;	
	pRS->divisor = 24;	
	pRS->blocks = 0;
	for(col=0;col<8;++col)
	{
		LPSetLED(7, col, LED_RED);				
		pRS->note[col] = 36+col;
	}
}

void rainstorm_done()
{
	free(pRS);
	pRS = NULL;
}

void rainstorm_event(int type, int row, int col)
{
	if(type == EVENT_KEYDOWN)	
	{
		if(row == 7 && col < 8)
		{
			pRS->cycle[col] = !pRS->cycle[col];
			LPSetLED(row, col, pRS->cycle[col] ? LED_GREEN : LED_RED);				
		}
		else if(col < 8)
		{
			if(pRS->blocks<10 && !pRS->state[row][col])
			{
				pRS->blocks++;
				pRS->state[row][col] = 1;
				LPSetLED(row, col, LED_YELLOW);				
			}
		}
	}
	else if(type == EVENT_TICK)
	{
		if((++pRS->ticks % pRS->divisor) == 0) 
		{		
			LPBeginBufferedUpdate();
			for(col = 0; col<8; ++col)
			{				
				byte newState[9];
								
				// shift all cells down
				for(row = 8; row>0; --row)
					newState[row] = pRS->state[row-1][col];
					
				// play note?
				if(newState[7])
					MIDIPlayNote((int)pRS->note[col], 127);
				else if(pRS->state[7][col])
					MIDIStopNote((int)pRS->note[col]);
					
				// need to move from row 7 back up to top?
				if(newState[8])
				{
					if(pRS->cycle[col]) 
					{
						newState[0] = newState[8];
					}
					else
					{
						newState[0] = 0;
						pRS->blocks--;
					}
				}
				else
				{
					newState[0] = 0;
				}
				
				// perform updates
				for(row = 0; row<8; ++row)
				{				
					if(newState[row] != pRS->state[row][col])
					{
						if(row < 7)
						{
							LPSetLED(row, col, newState[row] ? LED_YELLOW : 0);				
						}
						pRS->state[row][col] = newState[row];
					}
				}					
			}
			LPEndBufferedUpdate();
		}
	}
}
