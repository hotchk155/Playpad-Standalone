typedef unsigned char byte;
typedef struct _SMQ_MSG {
	byte status;
	byte param1;
	byte param2;	
} SMQ_MSG;

extern void SMQInit();
extern void SMQWrite(SMQ_MSG *msg);
extern void SMQRead(SMQ_MSG *msg);
extern int SMQWaiting();
