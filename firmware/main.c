/* Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
 * Software License Agreement
 * 
 * Texas Instruments (TI) is supplying this software for use solely and
 * exclusively on TI's microcontroller products. The software is owned by
 * TI and/or its suppliers, and is protected under applicable copyright
 * laws. You may not combine this software with "viral" open-source
 * software in order to form a larger program.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
 * NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
 * NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
 * CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 * 
 * This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
 */

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdbulk.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "usb_bulk_structs.h"
#include <string.h>

#define CRATE_WIDTH		5
#define CRATE_HEIGHT	4
#define CRATES_X		8
#define CRATES_Y		4
#define BUS_COUNT		4
#define BYTES_PER_PIXEL	3
#define CRATES_PER_BUS	8
#define BUS_ROWS		(CRATES_Y*CRATE_HEIGHT)
#define CRATE_COUNT		(CRATES_X*CRATES_Y)
#define CRATE_SIZE		(CRATE_WIDTH*CRATE_HEIGHT)
#define BUS_SIZE		(CRATES_PER_BUS*CRATE_SIZE*BYTES_PER_PIXEL)
unsigned const char const BOTTLE_MAP[CRATE_SIZE] = {
	 0,  1,  2,  3,  4,
	19,  8,  7,  6,  5,
	18,  9, 10, 11, 12,
	17, 16, 15, 14, 13
};

unsigned const char const CRATE_MAP[CRATE_COUNT] = {
	0x37, 0x35, 0x33, 0x31, 0x21, 0x23, 0x25, 0x27,
	0x36, 0x34, 0x32, 0x30, 0x20, 0x22, 0x24, 0x26,
	0x16, 0x14, 0x12, 0x10, 0x00, 0x02, 0x04, 0x06,
	0x17, 0x15, 0x13, 0x11, 0x01, 0x03, 0x05, 0x07
};

#define SYSTICKS_PER_SECOND		100
#define SYSTICK_PERIOD_MS		(1000 / SYSTICKS_PER_SECOND)

unsigned char framebuffer1[BUS_COUNT*BUS_SIZE];
unsigned char framebuffer2[BUS_COUNT*BUS_SIZE];
unsigned char *framebuffer_input = framebuffer1;
unsigned char *framebuffer_output = framebuffer2;

unsigned long framebuffer_read(void *data, unsigned long len);
/* Kick off DMA transfer from RAM to SPI interfaces */
void kickoff_transfers(void);
void kickoff_transfer(unsigned int channel, unsigned int offset, int base);
void ssi_udma_channel_config(unsigned int channel);

unsigned char ucControlTable[1024] __attribute__ ((aligned(1024)));

volatile unsigned long g_ulSysTickCount = 0;

#ifdef DEBUG
#define DEBUG_PRINT UARTprintf
#else
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

volatile unsigned long g_ulFlags = 0;
char *g_pcStatus;
static volatile tBoolean g_bUSBConfigured = false;

void SysTickIntHandler(void) {
	g_ulSysTickCount++;
}

unsigned long usb_rx_handler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue, void *pvMsgData) {
	unsigned int read;
	switch(ulEvent) {
		case USB_EVENT_CONNECTED:
			g_bUSBConfigured = true;
			DEBUG_PRINT("Host connected.\n");
			break;
		case USB_EVENT_DISCONNECTED:
			g_bUSBConfigured = false;
			DEBUG_PRINT("Host disconnected.\n");
			break;
		case USB_EVENT_RX_AVAILABLE:
			DEBUG_PRINT("Handling host data.\n");
			/* Beware of the cast, it might bite. */
			read = USBDBulkPacketRead((void *)&g_sBulkDevice, usb_rx_buffer, BULK_BUFFER_SIZE, 1);
			return framebuffer_read(usb_rx_buffer, read);
		case USB_EVENT_SUSPEND:
		case USB_EVENT_RESUME:
			break;
		default:
			break;
	}
	return 0;
}

typedef struct {
	unsigned char command; /* 0x00 for frame data, 0x01 to initiate latch */
	unsigned char crate_x;
	unsigned char crate_y;
	unsigned char rgb_data[CRATE_SIZE*BYTES_PER_PIXEL];
} FramebufferData;

unsigned long framebuffer_read(void *data, unsigned long len) {
	if(len < 1)
		goto length_error;
	DEBUG_PRINT("Rearranging data.\n");
	FramebufferData *fb = (FramebufferData *)data;
	if(fb->command == 1){
		if(len != 1)
			goto length_error;
		DEBUG_PRINT("Starting DMA.\n");
		kickoff_transfers();
	}else{
		if(len != sizeof(FramebufferData))
			goto length_error;

		if(fb->crate_x > CRATES_X || fb->crate_y > CRATES_Y){
			UARTprintf("Invalid frame index\n");
			return len;
		}

		unsigned int idx = CRATE_MAP[fb->crate_x + fb->crate_y*CRATES_X];
		unsigned int bus = idx>>4;
		unsigned int crate = idx & 0x0F;
		for(unsigned int x=0; x<CRATE_WIDTH; x++){
			for(unsigned int y=0; y<CRATE_HEIGHT; y++){
				unsigned int bottle	= BOTTLE_MAP[x + y*CRATE_WIDTH];
				unsigned int dst	= bus*BUS_SIZE + (crate*CRATE_SIZE + bottle)*3;
				unsigned int src	= (y*CRATE_WIDTH + x)*3;
				// Copy r, g and b data
				framebuffer_input[dst]	   = fb->rgb_data[src];
				framebuffer_input[dst + 1] = fb->rgb_data[src + 1];
				framebuffer_input[dst + 2] = fb->rgb_data[src + 2];
			}
		}
	}
	return len;
length_error:
	UARTprintf("Invalid packet length\n");
	return len;
}

void kickoff_transfers() {
    while(MAP_uDMAChannelIsEnabled(11)
		|| MAP_uDMAChannelIsEnabled(25)
		|| MAP_uDMAChannelIsEnabled(13)
		|| MAP_uDMAChannelIsEnabled(15)){
		UARTprintf("A DMA tranfer is still running\n");
		/* Idle for some time to give the µDMA controller a chance to complete its job */
		SysCtlDelay(5000);
	}
	/* Wait 1.2ms (20kCy @ 50MHz) to ensure the WS2801 latch this frame's data */
	SysCtlDelay(20000);
	/* Swap buffers */
	unsigned char *tmp = framebuffer_output;
	framebuffer_output = framebuffer_input;
	framebuffer_input = tmp;
	/* Re-schedule DMA transfers */
	kickoff_transfer(11, 0, SSI0_BASE);
	kickoff_transfer(25, 1, SSI1_BASE);
	kickoff_transfer(13, 2, SSI2_BASE);
	kickoff_transfer(15, 3, SSI3_BASE);
}

inline void kickoff_transfer(unsigned int channel, unsigned int offset, int base) {
	MAP_uDMAChannelTransferSet(channel | UDMA_PRI_SELECT, UDMA_MODE_BASIC, framebuffer_output+BUS_SIZE*offset, (void *)(base + SSI_O_DR), BUS_SIZE);
	MAP_uDMAChannelEnable(channel);
}

void ssi_udma_channel_config(unsigned int channel) {
	/* Set the USEBURST attribute for the uDMA SSI TX channel.	This will force the controller to always use a burst
	 * when transferring data from the TX buffer to the SSI.  This is somewhat more effecient bus usage than the default
	 * which allows single or burst transfers. */
	MAP_uDMAChannelAttributeEnable(channel, UDMA_ATTR_USEBURST);
	/* Configure the SSI Tx µDMA Channel to transfer from RAM to TX FIFO. The arbitration size is set to 4, which
	 * matches the SSI TX FIFO µDMA trigger threshold. */
	MAP_uDMAChannelControlSet(channel | UDMA_PRI_SELECT, UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_4);
}

int main(void) {
	/* Enable lazy stacking for interrupt handlers.  This allows floating-point instructions to be used within interrupt
	 * handlers, but at the expense of extra stack usage. */
	MAP_FPULazyStackingEnable();

	/* Set clock to PLL at 50MHz */
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
					   SYSCTL_XTAL_16MHZ);

	/* Configure UART0 pins */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
	MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
	MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	/* Enable the GPIO pins for the LED (PF2 & PF3). */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_2);

	UARTStdioInit(0);
	UARTprintf("Booting...\n\n");

	g_bUSBConfigured = false;

	/* Enable the GPIO peripheral used for USB, and configure the USB pins. */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	MAP_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	/* Enable the system tick. FIXME do we need this? */
	MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKS_PER_SECOND);
	MAP_SysTickIntEnable();
	MAP_SysTickEnable();

	/* Configure USB */
	USBStackModeSet(0, USB_MODE_FORCE_DEVICE, 0);
	USBDBulkInit(0, (tUSBDBulkDevice *)&g_sBulkDevice);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	MAP_GPIOPinConfigure(GPIO_PA5_SSI0TX);
	MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_5);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	MAP_GPIOPinConfigure(GPIO_PB4_SSI2CLK);
	MAP_GPIOPinConfigure(GPIO_PB7_SSI2TX);
	MAP_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_7);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	MAP_GPIOPinConfigure(GPIO_PD0_SSI3CLK);
	MAP_GPIOPinConfigure(GPIO_PD3_SSI3TX);
	MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_3);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	MAP_GPIOPinConfigure(GPIO_PF2_SSI1CLK);
	MAP_GPIOPinConfigure(GPIO_PF1_SSI1TX);
	MAP_GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_1);

	/* Configure SSI0..3 for the ws2801's SPI-like protocol */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

	/* 200kBd */
	MAP_SSIConfigSetExpClk(SSI0_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	MAP_SSIConfigSetExpClk(SSI1_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	MAP_SSIConfigSetExpClk(SSI2_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	MAP_SSIConfigSetExpClk(SSI3_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);

	/* Configure the µDMA controller for use by the SPI interface */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	MAP_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);
	// FIXME what do we need this for? IntEnable(INT_UDMAERR); // Enable µDMA error interrupt
	MAP_uDMAEnable();
	MAP_uDMAControlBaseSet(ucControlTable);
	
	MAP_uDMAChannelAssign(UDMA_CH11_SSI0TX);
	MAP_uDMAChannelAssign(UDMA_CH25_SSI1TX);
	MAP_uDMAChannelAssign(UDMA_CH13_SSI2TX);
	MAP_uDMAChannelAssign(UDMA_CH15_SSI3TX);
	
	ssi_udma_channel_config(11);
	ssi_udma_channel_config(25);
	ssi_udma_channel_config(13);
	ssi_udma_channel_config(15);

	MAP_SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);
	MAP_SSIDMAEnable(SSI1_BASE, SSI_DMA_TX);
	MAP_SSIDMAEnable(SSI2_BASE, SSI_DMA_TX);
	MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX);

	/* Enable the SSIs after configuring anything around them. */
	MAP_SSIEnable(SSI0_BASE);
	MAP_SSIEnable(SSI1_BASE);
	MAP_SSIEnable(SSI2_BASE);
	MAP_SSIEnable(SSI3_BASE);

	UARTprintf("Booted.\n");

	while(1);
}
