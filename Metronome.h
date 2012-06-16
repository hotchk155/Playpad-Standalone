typedef unsigned char byte;

typedef struct {
	VOS_HANDLE hTimer;
	unsigned int initTimer;	
	vos_mutex_t mAccess;
} METRONOME;

extern void MetroSetBPM(METRONOME *pMetro, int bpm);
extern void MetroInit(METRONOME *pMetro, byte device, int whichTimer);
extern void MetroStart(METRONOME *pMetro, byte device, int bpm);
extern void MetroDelay(METRONOME *pMetro);
