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
#define CRATES_X		4
#define CRATES_Y		2
#define BUS_COUNT		4
#define BYTES_PER_PIXEL	3
#define BUS_ROWS		(CRATES_Y*CRATE_HEIGHT)
#define CRATES_PER_BUS	(CRATES_X*CRATES_Y)
#define CRATE_SIZE		(CRATE_WIDTH*CRATE_HEIGHT)
#define BUS_SIZE		(CRATES_PER_BUS*CRATE_SIZE*BYTES_PER_PIXEL)
unsigned const char const BOTTLE_MAP[CRATE_SIZE] = {
	 0,  1,  2,  3,  4,
	19,  8,  7,  6,  5,
	18,  9, 10, 11, 12,
	17, 16, 15, 14, 13
};

unsigned const char const CRATE_MAP[CRATES_PER_BUS] = {
	6, 4, 2, 0,
	7, 5, 3, 1
};

#define SYSTICKS_PER_SECOND		100
#define SYSTICK_PERIOD_MS		(1000 / SYSTICKS_PER_SECOND)

unsigned char framebuffer[BUS_COUNT*BUS_SIZE];
unsigned long framebuffer_read(void *data, unsigned long len);
/* Kick off DMA transfer from RAM to SPI interfaces */
void kickoff_transfers(void);
void kickoff_transfer(unsigned int channel, unsigned int offset, int base);
void ssi_udma_channel_config(unsigned int channel);

unsigned char ucControlTable[1024] __attribute__ ((aligned(1024)));

volatile unsigned long g_ulSysTickCount = 0;

#ifdef DEBUG
unsigned long g_ulUARTRxErrors = 0;
#endif

//Debug output is available via UART0 if DEBUG is defined during build.
#ifdef DEBUG
//Map all debug print calls to UARTprintf in debug builds.
#define DEBUG_PRINT UARTprintf
#else
//Compile out all debug print calls in release builds.
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

volatile unsigned long g_ulFlags = 0;
char *g_pcStatus;
static volatile tBoolean g_bUSBConfigured = false;

void SysTickIntHandler(void) {
	g_ulSysTickCount++;
}

/* Will be called when a DMA transfer is complete */
void SSI0IntHandler(void) {
	/* FIXME is this necessary? */
    unsigned long ssistatus = ROM_SSIIntStatus(SSI0_BASE, 1);
    ROM_SSIIntClear(SSI0_BASE, ssistatus);

    if(!ROM_uDMAChannelIsEnabled(11)){
		/* A TX DMA transfer was completed */
		/* FIXME actually, just set a flag here and kick off when all four controllers signal completion.*/
		/* Wait 1.2ms for the WS2801s to latch (the datasheet specifies at least 500µs) */
		SysCtlDelay(20000);
		kickoff_transfers();
	}
}

unsigned long RxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue, void *pvMsgData) {
	switch(ulEvent) {
		case USB_EVENT_CONNECTED:
			g_bUSBConfigured = true;
			UARTprintf("Host connected.\n");
			USBBufferFlush(&g_sRxBuffer);
			break;
		case USB_EVENT_DISCONNECTED:
			g_bUSBConfigured = false;
			UARTprintf("Host disconnected.\n");
			break;
		case USB_EVENT_RX_AVAILABLE:
			UARTprintf("Handling host data.\n");
			USBBufferDataRemoved(&g_sRxBuffer, framebuffer_read(pvMsgData, ulMsgValue));
		case USB_EVENT_SUSPEND:
		case USB_EVENT_RESUME:
			break;
		default:
			break;
	}
	return 0;
}

typedef struct {
	unsigned char crate_x;
	unsigned char crate_y;
	unsigned char rgb_data[CRATE_SIZE*BYTES_PER_PIXEL];
} FramebufferData;

unsigned long framebuffer_read(void *data, unsigned long len) {
	if(len != sizeof(FramebufferData))
		return len;
	UARTprintf("Rearranging data.\n");
	FramebufferData *fb = (FramebufferData *)data;

	unsigned int bus = fb->crate_x/CRATES_X;
	fb->crate_x %= CRATES_X;

	if(bus > BUS_COUNT || fb->crate_y > CRATES_Y){
		UARTprintf("Invalid frame index\n");
		return len;
	}

	/* Mirror crate map for the display's right half */
	if(bus >= BUS_COUNT/2)
		fb->crate_x = CRATES_X - fb->crate_x - 1;

	unsigned int crate	= CRATE_MAP[fb->crate_x + fb->crate_y*CRATES_X];
	for(unsigned int x=0; x<CRATE_WIDTH; x++){
		for(unsigned int y=0; y<CRATE_HEIGHT; y++){
			unsigned int bottle	= BOTTLE_MAP[x + y*CRATE_WIDTH];
			unsigned int dst	= bus*BUS_SIZE + (crate*CRATE_SIZE + bottle)*3;
			unsigned int src	= (y*CRATE_WIDTH + x)*3;
			// Copy r, g and b data
			framebuffer[dst]	 = fb->rgb_data[src];
			framebuffer[dst + 1] = fb->rgb_data[src + 1];
			framebuffer[dst + 2] = fb->rgb_data[src + 2];
		}
	}

	UARTprintf("Starting DMA.\n");
	kickoff_transfers();
	return len;
}

void kickoff_transfers() {
	kickoff_transfer(11, 0, SSI0_BASE);
	kickoff_transfer(25, 1, SSI1_BASE);
	kickoff_transfer(13, 2, SSI2_BASE);
	kickoff_transfer(15, 3, SSI3_BASE);
}

void kickoff_transfer(unsigned int channel, unsigned int offset, int base) {
	ROM_uDMAChannelTransferSet(channel | UDMA_PRI_SELECT, UDMA_MODE_BASIC, framebuffer+BUS_SIZE*offset, (void *)(base + SSI_O_DR), BUS_SIZE);
	ROM_uDMAChannelEnable(channel);
}

void ssi_udma_channel_config(unsigned int channel) {
	/* Set the USEBURST attribute for the uDMA SSI TX channel.	This will force the controller to always use a burst
	 * when transferring data from the TX buffer to the SSI.  This is somewhat more effecient bus usage than the default
	 * which allows single or burst transfers. */
	ROM_uDMAChannelAttributeEnable(channel, UDMA_ATTR_USEBURST);
	/* Configure the SSI Tx µDMA Channel to transfer from RAM to TX FIFO. The arbitration size is set to 4, which
	 * matches the SSI TX FIFO µDMA trigger threshold. */
	ROM_uDMAChannelControlSet(channel | UDMA_PRI_SELECT, UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_4);
}

int main(void) {
	/* Enable lazy stacking for interrupt handlers.  This allows floating-point instructions to be used within interrupt
	 * handlers, but at the expense of extra stack usage. */
	ROM_FPULazyStackingEnable();

	/* Set clock to PLL at 50MHz */
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
					   SYSCTL_XTAL_16MHZ);

	/* Configure UART0 pins */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	/* Enable the GPIO pins for the LED (PF2 & PF3). */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_2);

	UARTStdioInit(0);
	UARTprintf("Booting...\n\n");

	g_bUSBConfigured = false;

	/* Enable the GPIO peripheral used for USB, and configure the USB pins. */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	/* Enable the system tick. FIXME do we need this? */
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();

	/* Configure USB */
	USBBufferInit((tUSBBuffer *)&g_sRxBuffer);
	USBStackModeSet(0, USB_MODE_FORCE_DEVICE, 0);
	USBDBulkInit(0, (tUSBDBulkDevice *)&g_sBulkDevice);

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_5);

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB4_SSI2CLK);
	GPIOPinConfigure(GPIO_PB7_SSI2TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_7);

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinConfigure(GPIO_PD0_SSI3CLK);
	GPIOPinConfigure(GPIO_PD3_SSI3TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_3);

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinConfigure(GPIO_PF2_SSI1CLK);
	GPIOPinConfigure(GPIO_PF1_SSI1TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_1);

	/* Configure SSI0..3 for the ws2801's SPI-like protocol */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

	/* 200kBd */
	SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	SSIConfigSetExpClk(SSI1_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	SSIConfigSetExpClk(SSI2_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
	SSIConfigSetExpClk(SSI3_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);

	/* Configure the µDMA controller for use by the SPI interface */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);
	// FIXME what do we need this for? ROM_IntEnable(INT_UDMAERR); // Enable µDMA error interrupt
	ROM_uDMAEnable();
	ROM_uDMAControlBaseSet(ucControlTable);
	
	ROM_uDMAChannelAssign(UDMA_CH11_SSI0TX);
	ROM_uDMAChannelAssign(UDMA_CH25_SSI1TX);
	ROM_uDMAChannelAssign(UDMA_CH13_SSI2TX);
	ROM_uDMAChannelAssign(UDMA_CH15_SSI3TX);
	
	ssi_udma_channel_config(11);
	ssi_udma_channel_config(25);
	ssi_udma_channel_config(13);
	ssi_udma_channel_config(15);

	ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);
	ROM_SSIDMAEnable(SSI1_BASE, SSI_DMA_TX);
	ROM_SSIDMAEnable(SSI2_BASE, SSI_DMA_TX);
	ROM_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX);

    ROM_IntEnable(INT_SSI0);

	/* Enable the SSIs after configuring anything around them. */
	ROM_SSIEnable(SSI0_BASE);
	ROM_SSIEnable(SSI1_BASE);
	ROM_SSIEnable(SSI2_BASE);
	ROM_SSIEnable(SSI3_BASE);

	UARTprintf("Booted.\n");

	while(1);
}
