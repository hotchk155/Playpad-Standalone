//////////////////////////////////////////////////////////////////////
//
// DUAL USB HOST FOR NOVATION LAUNCHPAD <-> MIDI TRANSFER
//
//////////////////////////////////////////////////////////////////////

//
// INCLUDE FILES
//
#include "Playpad.h"
#include "USBHostGenericDrv.h"
#include "Metronome.h"
#include "SynchMIDIQueue.h"
#include "Playpad.h"


//
// TYPE DEFS
//
typedef struct {
	VOS_HANDLE hUSBHOST;
	VOS_HANDLE hUSBHOSTGENERIC;
	unsigned char uchDeviceNumber;
	unsigned char uchMidiChannel;
	unsigned char uchActivityLed;
	unsigned char uchAttached;
} HOST_PORT_DATA;


//
// VARIABLE DECL
//
vos_tcb_t *tcbSetup;
vos_tcb_t *tcbHostA;
vos_tcb_t *tcbHostB;
vos_tcb_t *tcbPlaypad;
vos_tcb_t *tcbMetro;
vos_tcb_t *tcbMIDIInput;

vos_semaphore_t setupSem;

VOS_HANDLE hGpioA;
VOS_HANDLE hUART;


HOST_PORT_DATA PortA = {0};
HOST_PORT_DATA PortB = {0};
	
METRONOME metro;
	
//
// FUNCTION PROTOTYPES
//
void Setup();
void RunHostPort(HOST_PORT_DATA *pHostData);
void RunMetronome();
void RunPlaypad();
void RunMIDIInput();


//////////////////////////////////////////////////////////////////////
//
// IOMUX SETUP
// ALWAYS VNC2 32PIN PACKAGE
//
//////////////////////////////////////////////////////////////////////
void iomux_setup()
{
	// Debugger to pin 11 as Bi-Directional.
	vos_iomux_define_bidi(199, IOMUX_IN_DEBUGGER, IOMUX_OUT_DEBUGGER);
	// GPIO_Port_A_1 to pin 12 as Output.
	vos_iomux_define_output(12, IOMUX_OUT_GPIO_PORT_A_1);
	// GPIO_Port_A_2 to pin 14 as Output.
	vos_iomux_define_output(14, IOMUX_OUT_GPIO_PORT_A_2);
	// GPIO_Port_A_3 to pin 15 as Output.
	vos_iomux_define_output(15, IOMUX_OUT_GPIO_PORT_A_3);
	// UART_TXD to pin 23 as Output.
	vos_iomux_define_output(23, IOMUX_OUT_UART_TXD);
	// UART_RXD to pin 24 as Input.
	vos_iomux_define_input(24, IOMUX_IN_UART_RXD);
	// UART_RTS_N to pin 25 as Output.
	vos_iomux_define_output(25, IOMUX_OUT_UART_RTS_N);
	// UART_CTS_N to pin 26 as Input.
	vos_iomux_define_input(26, IOMUX_IN_SPI_SLAVE_0_CS);
	// SPI_Slave_0_CLK to pin 29 as Input.
	vos_iomux_define_input(29, IOMUX_IN_SPI_SLAVE_0_CLK);
	// SPI_Slave_0_MOSI to pin 30 as Input.
	vos_iomux_define_input(30, IOMUX_IN_SPI_SLAVE_0_MOSI);
	// SPI_Slave_0_MISO to pin 31 as Output.
	vos_iomux_define_output(31, IOMUX_OUT_SPI_SLAVE_0_MISO);
	// GPIO_Port_A_1 to pin 32 as Output.
	//vos_iomux_define_output(32, IOMUX_OUT_GPIO_PORT_A_3);
}


//////////////////////////////////////////////////////////////////////
//
// MAIN
//
//////////////////////////////////////////////////////////////////////
void main(void)
{
	/* FTDI:SDD Driver Declarations */
	// UART Driver configuration context
	uart_context_t uartContext;
	// USB Host configuration context
	usbhost_context_t usbhostContext;
	gpio_context_t gpioCtx;

	// Kernel initialisation
	vos_init(50, VOS_TICK_INTERVAL, VOS_NUMBER_DEVICES);
	vos_set_clock_frequency(VOS_48MHZ_CLOCK_FREQUENCY);
	vos_set_idle_thread_tcb_size(512);

	// Set up the io multiplexing
	iomux_setup();

	// Initialise GPIO port A
	gpioCtx.port_identifier = GPIO_PORT_A;
	gpio_init(VOS_DEV_GPIO_A,&gpioCtx); //

	// Initialise UART
	uartContext.buffer_size = VOS_BUFFER_SIZE_128_BYTES;
	uart_init(VOS_DEV_UART, &uartContext);
	

	// Initialise USB Host devices
	usbhostContext.if_count = 8;
	usbhostContext.ep_count = 16;
	usbhostContext.xfer_count = 2;
	usbhostContext.iso_xfer_count = 2;
	//usbhost_init(VOS_DEV_USBHOST_1, VOS_DEV_USBHOST_2, &usbhostContext);
	usbhost_init(VOS_DEV_USBHOST_1, -1, &usbhostContext);

	// Initialise the USB function device
	usbhostGeneric_init(VOS_DEV_USBHOSTGENERIC_1);
	//usbhostGeneric_init(VOS_DEV_USBHOSTGENERIC_2);


	PortA.uchDeviceNumber = VOS_DEV_USBHOSTGENERIC_1;
	PortA.uchMidiChannel = 0;
	PortA.uchActivityLed = 0b00000010;

	//PortB.uchDeviceNumber = VOS_DEV_USBHOSTGENERIC_2;
	//PortB.uchMidiChannel = 1;
	//PortB.uchActivityLed = 0b00000100;

	// setup the event queue
	SMQInit();

	// Initializes our device with the device manager.
	
	tcbSetup = vos_create_thread_ex(10, 1024, Setup, "Setup", 0);
	tcbHostA = vos_create_thread_ex(20, 1024, RunHostPort, "RunHostPortA", sizeof(HOST_PORT_DATA*), &PortA);
	//tcbHostB = vos_create_thread_ex(20, 1024, RunHostPort, "RunHostPortB", sizeof(HOST_PORT_DATA*), &PortB);
	tcbPlaypad = vos_create_thread_ex(15, 1024, RunPlaypad, "RunPlaypad", 0);
	tcbMetro = vos_create_thread_ex(15, 1024, RunMetronome, "RunMetronome", 0);
	tcbMIDIInput = vos_create_thread_ex(15, 1024, RunMIDIInput, "RunMIDIInput", 0);

	vos_init_semaphore(&setupSem,0);
	
	// And start the thread
	vos_start_scheduler();

main_loop:
	goto main_loop;
}


//////////////////////////////////////////////////////////////////////
//
// APPLICATION SETUP THREAD FUNCTION
//
//////////////////////////////////////////////////////////////////////
void Setup()
{
	common_ioctl_cb_t spis_iocb;
	usbhostGeneric_ioctl_t generic_iocb;
	gpio_ioctl_cb_t gpio_iocb;
	common_ioctl_cb_t uart_iocb;
	unsigned char uchLeds;

	// Open up the base level drivers
	hGpioA  	= vos_dev_open(VOS_DEV_GPIO_A);
	PortA.hUSBHOST = vos_dev_open(VOS_DEV_USBHOST_1);
	//PortB.hUSBHOST = vos_dev_open(VOS_DEV_USBHOST_2);

	gpio_iocb.ioctl_code = VOS_IOCTL_GPIO_SET_MASK;
	gpio_iocb.value = 0b00001110;
	vos_dev_ioctl(hGpioA, &gpio_iocb);
	uchLeds = 0b00001000;
	vos_dev_write(hGpioA,&uchLeds,1,NULL);

	hUART = vos_dev_open(VOS_DEV_UART);

	/* UART ioctl call to enable DMA and link to DMA driver */
	uart_iocb.ioctl_code = VOS_IOCTL_COMMON_ENABLE_DMA;
	vos_dev_ioctl(hUART, &uart_iocb);

	// Set up for MIDI
    uart_iocb.ioctl_code = VOS_IOCTL_UART_SET_BAUD_RATE;
	uart_iocb.set.uart_baud_rate = 31250;
	vos_dev_ioctl(hUART, &uart_iocb);

    uart_iocb.ioctl_code = VOS_IOCTL_UART_SET_FLOW_CONTROL;
	uart_iocb.set.param = UART_FLOW_NONE;
	vos_dev_ioctl(hUART, &uart_iocb);

	// Set up the metronome
	MetroInit(&metro, VOS_DEV_TIMER0, TIMER_0);
	
	// Release other application threads
	vos_signal_semaphore(&setupSem);
}

//////////////////////////////////////////////////////////////////////
//
// SET LED
//
//////////////////////////////////////////////////////////////////////
void setLed(unsigned char mask, unsigned char value)
{
	unsigned char leds;
	vos_dev_read(hGpioA,&leds,1,NULL);
	if(value)
		leds |= mask;
	else
		leds &= ~mask;
	vos_dev_write(hGpioA,&leds,1,NULL);
}
	
//////////////////////////////////////////////////////////////////////
//
// Get connect state
//
//////////////////////////////////////////////////////////////////////
unsigned char usbhost_connect_state(VOS_HANDLE hUSB)
{
	unsigned char connectstate = PORT_STATE_DISCONNECTED;
	usbhost_ioctl_cb_t hc_iocb;

	if (hUSB)
	{
		hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_GET_CONNECT_STATE;
		hc_iocb.get = &connectstate;
		vos_dev_ioctl(hUSB, &hc_iocb);
	}

	return connectstate;
}



//////////////////////////////////////////////////////////////////////
//
// THREAD FUNCTION TO RUN USB HOST PORT
//
//////////////////////////////////////////////////////////////////////
void RunHostPort(HOST_PORT_DATA *pHostData)
{
	unsigned char status;
	unsigned char buf[64];
	unsigned short num_read;
	unsigned int handle;
	usbhostGeneric_ioctl_t generic_iocb;
	usbhostGeneric_ioctl_cb_attach_t genericAtt;
	usbhost_device_handle_ex ifDev;
	usbhost_ioctl_cb_t hc_iocb;
	usbhost_ioctl_cb_vid_pid_t hc_iocb_vid_pid;
	gpio_ioctl_cb_t gpio_iocb;

	SMQ_MSG msg;
	msg.status = 0x90;

	// wait for setup to complete
	vos_wait_semaphore(&setupSem);
	vos_signal_semaphore(&setupSem);

	// loop forever
	while(1)
	{
		// is the device enumerated on this port?
		if (usbhost_connect_state(pHostData->hUSBHOST) == PORT_STATE_ENUMERATED)
		{
			// user ioctl to find first hub device
			hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_DEVICE_GET_NEXT_HANDLE;
			hc_iocb.handle.dif = NULL;
			hc_iocb.set = NULL;
			hc_iocb.get = &ifDev;
			if (vos_dev_ioctl(pHostData->hUSBHOST, &hc_iocb) == USBHOST_OK)
			{

				hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_DEVICE_GET_VID_PID;
				hc_iocb.handle.dif = ifDev;
				hc_iocb.get = &hc_iocb_vid_pid;
				
				// attach the Launchpad device to the USB host device
				pHostData->hUSBHOSTGENERIC = vos_dev_open(pHostData->uchDeviceNumber);
				genericAtt.hc_handle = pHostData->hUSBHOST;
				genericAtt.ifDev = ifDev;
				generic_iocb.ioctl_code = VOS_IOCTL_USBHOSTGENERIC_ATTACH;
				generic_iocb.set.att = &genericAtt;
				if (vos_dev_ioctl(pHostData->hUSBHOSTGENERIC, &generic_iocb) == USBHOSTGENERIC_OK)
				{
					setLed(pHostData->uchActivityLed, 1);
					
					// now we loop until the launchpad is detached
					while(1)
					{
						int pos = 0;
						byte param = 1;
						
						// read data
						status = vos_dev_read(pHostData->hUSBHOSTGENERIC, buf, 64, &num_read);
						if(status != USBHOSTGENERIC_OK)
							break;
							
						setLed(pHostData->uchActivityLed, 0);

						// interpret MIDI data and pass to application thread
						
						while(pos < num_read)
						{
							if(buf[pos] & 0x80)
							{
								msg.status = buf[pos];
								param = 1;
							}
							else if(param == 1)
							{
								msg.param1 = buf[pos];
								param = 2;
							}
							else if(param == 2)
							{
								msg.param2 = buf[pos];
								SMQWrite(&msg);										
								param = 1;
							}
							++pos;
						}
						
						setLed(pHostData->uchActivityLed, 1);
					}					
				}
				vos_dev_close(pHostData->hUSBHOSTGENERIC);
			}
		}
		setLed(pHostData->uchActivityLed, 0);
	}
}	

//////////////////////////////////////////////////////////////////////
//
// THREAD FUNCTION TO RUN MIDI INPUT
//
//////////////////////////////////////////////////////////////////////
void RunMIDIInput()
{
	SMQ_MSG msg;
	unsigned short num_read;
	byte uch;
	
	// wait for setup to complete
	vos_wait_semaphore(&setupSem);
	vos_signal_semaphore(&setupSem);
	
	for(;;)
	{
		vos_dev_read(hUART, &uch, 1, &num_read);
		if(num_read > 0)
		{
			if(uch == 0xf8)
			{
				msg.status = 0xf8;
				msg.param1 = 0x00;
				msg.param2 = 0x00;
				SMQWrite(&msg);
			}
		}
	}
	
}
	
//////////////////////////////////////////////////////////////////////
// Write raw data to MIDI UART
void MIDIWrite(byte status, byte param1, byte param2)
{
	byte msg[3];
	msg[0] = status;
	msg[1] = param1;
	msg[2] = param2;
	vos_dev_write(hUART, msg, 3, NULL);	
}
	
//////////////////////////////////////////////////////////////////////
// Write raw data to Launchpad
void LPWrite(byte status, byte param1, byte param2)
{
	byte msg[3];
	msg[0] = status;
	msg[1] = param1;
	msg[2] = param2;
	vos_dev_write(PortA.hUSBHOSTGENERIC, msg, 3, NULL);	
}
		
//////////////////////////////////////////////////////////////////////
// Send a MIDI note message to Launchpad to set/clear a LED
void LPSetLED(int row, int col, int colour)
{
	LPWrite(0x90, (byte)(row *16 + col), (byte)colour);
}

//////////////////////////////////////////////////////////////////////
// Start updating the LEDs
void LPBeginBufferedUpdate()
{
	// copy displayed data from buffer 0 (displayed) into buffer 1
	LPWrite(0xb0, 0x00, 0b0110100);

	// start updating buffer 0, display buffer 1
	LPWrite(0xb0, 0x00, 0b0100001);
	
}
	
//////////////////////////////////////////////////////////////////////
// Finisg updating the LEDs
void LPEndBufferedUpdate()
{
	// display and update buffer 0
	LPWrite(0xb0, 0x00, 0b0100000);	
}
	
	/*
Bit Name Meaning
6 Must be 0.
5 Must be 1.
4 Copy If 1: copy the LED states from the new ‘displayed’
buffer to the new ‘updating’ buffer.
3 Flash If 1: continually flip ‘displayed’ buffers to make
selected LEDs flash.
2 Update Set buffer 0 or buffer 1 as the new ‘updating’ buffer.
1 Must be 0.
0 Display Set buffer 0 or buffer 1 as the new ‘displaying’
buffer
	*/
//////////////////////////////////////////////////////////////////////
// Play a note
void MIDIPlayNote(byte note, byte velocity)
{
	MIDIWrite(0x90, note, velocity);
}
		
//////////////////////////////////////////////////////////////////////
// Stop a note
void MIDIStopNote(byte note)
{
	MIDIWrite(0x90, note, 0x00);
}

/////////////////////////////////////////////////////////////////////
// Thread to dispatch tick events to the playpad thread
// at timed intervals
void RunMetronome() 
{
	SMQ_MSG tick;		
	
	// wait for setup to complete
	vos_wait_semaphore(&setupSem);
	vos_signal_semaphore(&setupSem);
	
	// Prepare standard tick message
	tick.status = 0xf8;
	tick.param1 = 0x00;
	tick.param2 = 0x00;
	
	// Start the metronome
	MetroStart(&metro, VOS_DEV_TIMER0, 120);
	for(;;) 
	{
		// run!
		SMQWrite(&tick);
		MetroDelay(&metro);
	}
}
		
////////////////////////////////////////////////////////////////////
// Main application thread
void RunPlaypad()
{
	SMQ_MSG msg;	
	
	// wait for setup to complete
	vos_wait_semaphore(&setupSem);
	vos_signal_semaphore(&setupSem);
		
	arpie_init();
	
	for(;;)
	{
		SMQRead(&msg);
		switch(msg.status)
		{
			case 0x80:
			case 0x90:
				if(msg.status == 0x80 || msg.param2 == 0)
				{
					arpie_event(EVENT_KEYUP, msg.param1/16, msg.param1%16);
				}
				else
				{
					arpie_event(EVENT_KEYDOWN, msg.param1/16, msg.param1%16);
				}
				break;
			case 0xf8: // clock tick
				arpie_event(EVENT_TICK, 0, 0);
				break;
		}
	}
}

