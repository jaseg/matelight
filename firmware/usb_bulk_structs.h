//*****************************************************************************
//
// usb_bulk_structs.h - Data structures defining this bulk USB device.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#ifndef _USB_BULK_STRUCTS_H_
#define _USB_BULK_STRUCTS_H_

//*****************************************************************************
//
// The size of the transmit and receive buffers used. 256 is chosen pretty
// much at random though the buffer should be at least twice the size of
// a maximum-sized USB packet.
//
//*****************************************************************************
#define BULK_BUFFER_SIZE 256

extern unsigned long RxHandler(void *pvCBData, unsigned long ulEvent,
                               unsigned long ulMsgValue, void *pvMsgData);

extern const tUSBBuffer g_sRxBuffer;
extern const tUSBDBulkDevice g_sBulkDevice;
extern unsigned char g_pucUSBRxBuffer[];

#endif
