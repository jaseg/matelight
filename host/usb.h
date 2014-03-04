#ifndef __USB_H__
#define __USB_H__

#include <libusb-1.0/libusb.h>

#define MATELIGHT_VID					0x1cbe
#define MATELIGHT_PID					0x0003

#define CRATE_WIDTH						5
#define CRATE_HEIGHT					4

#define MATELIGHT_FRAMEDATA_ENDPOINT	0x01
#define MATELIGHT_TIMEOUT				1000

#define GAMMA							2.5F

typedef struct {
	libusb_device_handle *handle;
	char *serial;
} matelight_handle;

int matelight_usb_init(void);
void matelight_usb_destroy(void);
matelight_handle *matelight_open(void);
int matelight_send_frame(matelight_handle *ml, void *buf, size_t w, size_t h, float brightness, int alpha);

#endif//__USB_H__
