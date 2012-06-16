#include "stdlib.h"
#include "string.h" //memset
#include "Playpad.h"

	
typedef struct {
	byte note[48];
	byte status[48];
	byte pattern[16];
	unsigned long ticks;
	int quantum;
	int index;
	byte shift;
	
	int nextRed;
	int nextGreen;
	
	byte redNote;
	byte greenNote;
} ARPIE_DATA;
// 4 * 8
	
// lower 2 rows = 16 step pattern
// top 6 rows = notes
// green = play every time	
// 
	
/*
	step sequencer
	YELLOW = play next red note and next green note together
	RED = next red note
	GREEN = next green note
	
*/
	
ARPIE_DATA *pData = NULL;
void arpie_init()
{
	int row;
	int col;
	int note;
	
	pData = (ARPIE_DATA*)malloc(sizeof(ARPIE_DATA));
	memset(pData,0,sizeof(ARPIE_DATA));
	pData->quantum = 6;
	
	LPSetLED(5, 8, LED_YELLOW);				
	LPSetLED(6, 8, LED_GREEN);				
	LPSetLED(7, 8, LED_RED);					
	
	LPSetLED(6, 0, LED_YELLOW);					
	
	note = 36;
	for(row = 5; row>=0; --row)
		for(col= 0; col<8;++col)
			pData->note[col + 8*row] = note++;
			
	pData->nextRed = -1;
	pData->nextGreen = -1;
}

void arpie_done()
{
	free(pData);
	pData = NULL;
}

void playstuff()
{
	int i;
				if(pData->greenNote)
				{
					MIDIStopNote(pData->greenNote);
					pData->greenNote = 0;									
				}
				if((pData->pattern[pData->index] & LED_GREEN) == LED_GREEN)
				{
					for(i=0;i<48;++i)
					{
						pData->nextGreen = (pData->nextGreen+1)%48;
						if((pData->status[pData->nextGreen] & LED_GREEN) == LED_GREEN) // red or yellow
						{
							pData->greenNote = pData->note[pData->nextGreen];					
							MIDIPlayNote(pData->greenNote, 127);
							break;
						}
					}
					if(i ==48)
						pData->nextGreen = -1;
				}
				
				if(pData->redNote)
				{
					MIDIStopNote(pData->redNote);
					pData->redNote = 0;									
				}
				if((pData->pattern[pData->index] & LED_RED) == LED_RED)
				{
					for(i=0;i<48;++i)
					{
						pData->nextRed = (pData->nextRed + 1)%48;
						if((pData->status[pData->nextRed] & LED_RED) == LED_RED) // red or yellow
						{
							pData->redNote = pData->note[pData->nextRed];					
							MIDIPlayNote(pData->redNote, 127);
							break;
						}
					}
					if(i==48)
						pData->nextRed = -1;
				}
}
	
void arpie_event(int type, int row, int col)
{
	int i;
	switch(type)
	{
		case EVENT_KEYDOWN:
			if(col == 8)
			{
				if(row == 5)
					pData->shift = LED_YELLOW;
				else if(row == 6)
					pData->shift = LED_GREEN;
				else if(row == 7)
					pData->shift = LED_RED;
				else
					pData->shift = 0;
			}	
			else if(row >= 6)
			{
				i = 8 * (row-6) + col;
				if(pData->shift && pData->pattern[i] != pData->shift)
					pData->pattern[i] = pData->shift;
				else
					pData->pattern[i] = pData->pattern[i]? 0 : LED_GREEN;
				LPSetLED(row,col,pData->pattern[i]);
			}
			else if(col < 8)
			{
				i = 8 * row + col;
				if(pData->shift && pData->status[i] != pData->shift)
					pData->status[i]  = pData->shift;
				else
					pData->status[i] = pData->status[i] ? 0 : LED_GREEN;
				LPSetLED(row,col,pData->status[i]);
			}
			break;
		case EVENT_KEYUP:
			if(col == 8)
			{
				pData->shift = 0;
			}			
			break;
		case EVENT_TICK:
			if(!(++pData->ticks % pData->quantum))
			{
				
				// Move the pattern sequencer
				row = 6 + pData->index/8;
				col = pData->index%8;				
				LPSetLED(row, col, pData->pattern[pData->index]);					
				if(++pData->index >= 16)
					pData->index = 0;
				row = 6 + pData->index/8;
				col = pData->index%8;				
				LPSetLED(row, col, LED_YELLOW);					
				
				playstuff();
				
			}
			break;
	}
}
