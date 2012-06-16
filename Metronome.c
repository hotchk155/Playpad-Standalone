#include "vos.h"
#include "Timers.h"
#include "Metronome.h"

///////////////////////////////////////////////////////////////////////////
// SET INITIAL TIMER COUNT
static void setTimerInit(METRONOME *pMetro) 
{
	if(pMetro->initTimer != 0)
	{
		tmr_ioctl_cb_t tmr_iocb;
		tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_SET_COUNT;
		tmr_iocb.param = pMetro->initTimer; 
		vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
		pMetro->initTimer = 0;
	}
}

///////////////////////////////////////////////////////////////////////////
// SET BPM
void MetroSetBPM(METRONOME *pMetro, int bpm)
{
	vos_lock_mutex(&pMetro->mAccess);	
	pMetro->initTimer = 2500/bpm;
	vos_unlock_mutex(&pMetro->mAccess);	
}

///////////////////////////////////////////////////////////////////////////
// INITIALISE
void MetroInit(METRONOME *pMetro, byte device, int whichTimer)
{
	tmr_context_t tmrCtx;
	vos_init_mutex(&pMetro->mAccess,0);	
	
	// Initialise Timer
	tmrCtx.timer_identifier = whichTimer;
	tmr_init(device, &tmrCtx);	
}

///////////////////////////////////////////////////////////////////////////
// START
void MetroStart(METRONOME *pMetro, byte device, int bpm)
{
	tmr_ioctl_cb_t tmr_iocb;
		
	// open the device
	pMetro->hTimer = vos_dev_open(device);

	// setup start scalar from bpm
	MetroSetBPM(pMetro, bpm);
	
	// Set the timer tick
	tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_SET_TICK_SIZE;
	tmr_iocb.param = TIMER_TICK_MS;
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);

	// set direction
	tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_SET_DIRECTION;
	tmr_iocb.param = TIMER_COUNT_DOWN;
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
	
	// set mode
	tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_SET_MODE;
	tmr_iocb.param = TIMER_MODE_CONTINUOUS;
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
	
	// start the timer
	tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_START;
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
}

///////////////////////////////////////////////////////////////////////////
// WAIT FOR METRONOME TICK
void MetroDelay(METRONOME *pMetro)
{
	// wait for timer to tick
	tmr_ioctl_cb_t tmr_iocb;
	tmr_iocb.ioctl_code = VOS_IOCTL_TIMER_WAIT_ON_COMPLETE;
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
	
	// change timer initialiser if needed
	vos_lock_mutex(&pMetro->mAccess);		
	if(pMetro->initTimer != 0)
		setTimerInit(pMetro);
	vos_unlock_mutex(&pMetro->mAccess);			
}

///////////////////////////////////////////////////////////////////////////
// START OR STOP 
void MetroControl(METRONOME *pMetro, int start)
{
	tmr_ioctl_cb_t tmr_iocb;
	tmr_iocb.ioctl_code = start? VOS_IOCTL_TIMER_START : VOS_IOCTL_TIMER_STOP;
	vos_lock_mutex(&pMetro->mAccess);		
	vos_dev_ioctl(pMetro->hTimer, &tmr_iocb);
	vos_unlock_mutex(&pMetro->mAccess);			
}
	
