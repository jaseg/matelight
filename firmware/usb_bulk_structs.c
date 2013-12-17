//*****************************************************************************
//
// usb_bulk_structs.c - Data structures defining this bulk USB device.
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

#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdbulk.h"
#include "usb_bulk_structs.h"

//*****************************************************************************
//
// The languages supported by this device.
//
//*****************************************************************************
const unsigned char g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

//*****************************************************************************
//
// The manufacturer string.
//
//*****************************************************************************
const unsigned char g_pManufacturerString[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'G', 0, 'o', 0, 'l', 0, 'd', 0, ' ', 0, '&', 0, ' ', 0, 'A', 0, 'p', 0,
    'p', 0, 'l', 0, 'e', 0
};

//*****************************************************************************
//
// The product string.
//
//*****************************************************************************
const unsigned char g_pProductString[] =
{
    (10 + 1) * 2,
    USB_DTYPE_STRING,
    'M', 0, 'a', 0, 't', 0, 'e', 0, ' ', 0, 'L', 0, 'i', 0, 'g', 0, 'h', 0,
    't', 0
};

//*****************************************************************************
//
// The serial number string.
//
//*****************************************************************************
const unsigned char g_pSerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '3', 0, '2', 0
};

//*****************************************************************************
//
// The data interface description string.
//
//*****************************************************************************
const unsigned char g_pDataInterfaceString[] =
{
    (19 + 1) * 2,
    USB_DTYPE_STRING,
    'B', 0, 'u', 0, 'l', 0, 'k', 0, ' ', 0, 'D', 0, 'a', 0, 't', 0,
    'a', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0,
    'a', 0, 'c', 0, 'e', 0
};

//*****************************************************************************
//
// The configuration description string.
//
//*****************************************************************************
const unsigned char g_pConfigString[] =
{
    (23 + 1) * 2,
    USB_DTYPE_STRING,
    'B', 0, 'u', 0, 'l', 0, 'k', 0, ' ', 0, 'D', 0, 'a', 0, 't', 0,
    'a', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0,
    'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0
};

//*****************************************************************************
//
// The descriptor string table.
//
//*****************************************************************************
const unsigned char * const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString,
    g_pDataInterfaceString,
    g_pConfigString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /                \
                                sizeof(unsigned char *))

//*****************************************************************************
//
// The bulk device initialization and customization structures. In this case,
// we are using USBBuffers between the bulk device class driver and the
// application code. The function pointers and callback data values are set
// to insert a buffer in each of the data channels, transmit and receive.
//
// With the buffer in place, the bulk channel callback is set to the relevant
// channel function and the callback data is set to point to the channel
// instance data. The buffer, in turn, has its callback set to the application
// function and the callback data set to our bulk instance structure.
//
//*****************************************************************************

tBulkInstance bulk_instance;
unsigned char usb_rx_buffer[BULK_BUFFER_SIZE];

const tUSBDBulkDevice g_sBulkDevice =
{
    USB_VID_STELLARIS,
    USB_PID_BULK,
    500,
    USB_CONF_ATTR_SELF_PWR,
    usb_rx_handler,
    0,
    0,
    0,
    g_pStringDescriptors,
    NUM_STRING_DESCRIPTORS,
	&bulk_instance
};

