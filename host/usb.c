
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libusb-1.0/libusb.h>

#include "color.h"
#include "usb.h"

int matelight_usb_init(){
	int rc = 0;
	rc = libusb_init(NULL);
	if(rc){
		fprintf(stderr, "LIBML: Cannot initialize libusb\n");
		return 1;
	}
	return 0;
}

void matelight_usb_destroy(){
	libusb_exit(NULL);
}

static int matelight_cmp_str_desc(libusb_device_handle *dev, uint8_t index, char *value){
	char data[256];
	if(libusb_get_string_descriptor_ascii(dev, index, (unsigned char*)data, sizeof(data)) < 0){
		fprintf(stderr, "LIBML: Cannot read device string descriptor\n");
		return 0;
	}
	return strcmp(data, value) == 0;
}

matelight_handle *matelight_open(){
	matelight_handle *out = NULL;
	libusb_device** device_list;
	struct libusb_device_descriptor desc;
	ssize_t res = libusb_get_device_list(NULL, &device_list);
	if(res == 0){
		fprintf(stderr, "LIBML: Cannot find any connected matelight\n");
		goto error;
	}else if(res < 0){
		fprintf(stderr, "LIBML: Error enumerating connected USB devices\n");
		goto error;
	}else{
		out = calloc(res+1, sizeof(matelight_handle));
		if(!out){
			fprintf(stderr, "LIBML: Cannot allocate memory\n");
			goto error;
		}
		memset(out, 0, (res+1)*sizeof(matelight_handle));
		unsigned int found = 0;
		for(ssize_t i=0; i<res; i++){
			libusb_get_device_descriptor(device_list[i], &desc);
			if(desc.idVendor == MATELIGHT_VID && desc.idProduct == MATELIGHT_PID){
				libusb_device_handle *handle;
				if(libusb_open(device_list[i], &(handle))){
					fprintf(stderr, "LIBML: Cannot open Mate Light USB device\n");
					goto error;
				}
				out[found].handle = handle;
				if(matelight_cmp_str_desc(handle, desc.iManufacturer, "Gold & Apple"))
				if(matelight_cmp_str_desc(handle, desc.iProduct, "Mate Light")){
#define BUF_SIZE 256
					char *serial = malloc(BUF_SIZE);
					if(!serial){
						fprintf(stderr, "LIBML: Cannot allocate memory\n");
						goto error;
					}
					if(libusb_get_string_descriptor_ascii(out[found].handle, desc.iSerialNumber, (unsigned char*)serial, BUF_SIZE) < 0){
						fprintf(stderr, "LIBML: Cannot read device string descriptor\n");
						goto error;
					}
#undef BUF_SIZE
					out[found].serial = serial;
					found++;
				}
			}
		}
		out[found].handle = NULL;
		out[found].serial = NULL;
	}
	libusb_free_device_list(device_list, 1);
	return out;
error:
	if(res>0 && out){
		for(ssize_t i=0; i<res; i++){
			if(out[i].handle)
				libusb_close(out[i].handle);
			free(out[i].serial);
		}
	}
	free(out);
	libusb_free_device_list(device_list, 1);
	return 0;
}

typedef struct{
	uint8_t opcode;
	uint8_t x, y;
	rgb_t buf[CRATE_WIDTH*CRATE_HEIGHT];
} crate_frame_t;

int matelight_send_frame(matelight_handle *ml, void *buf, size_t w, size_t h, float brightness, int alpha){
//	fprintf(stderr, "\nFRAME START\n");
	for(size_t cy=0; cy<h; cy++){
//		fprintf(stderr, "##### NEXT ROW\n");
		for(size_t cx=0; cx<w; cx++){
			crate_frame_t frame;
			frame.opcode = 0;
			frame.x = cx;
			frame.y = cy;
//			fprintf(stderr, "CRATE %d %d\n", cx, cy);
			for(size_t x=0; x<CRATE_WIDTH; x++){
				for(size_t y=0; y<CRATE_HEIGHT; y++){
					size_t dpos = y*CRATE_WIDTH + x;
					size_t spos = w*CRATE_WIDTH*(cy*CRATE_HEIGHT+y) + cx*CRATE_WIDTH+x;
					color_t *src = (((color_t*)buf)+spos);
					rgb_t *dst = frame.buf+dpos;
					/* Gamma correction */
#define GAMMA_APPLY(c) ((uint8_t)roundf(powf((c/255.0F), GAMMA) * brightness * 255))
					dst->r = GAMMA_APPLY(src->r);
					dst->g = GAMMA_APPLY(src->g);
					dst->b = GAMMA_APPLY(src->b);
//					fprintf(stderr, "%c", ((dst->r > 1) ? '#' : '.'));
#undef GAMMA_APPLY
				}
//				fprintf(stderr, "\n");
			}
			int transferred = 0;
			int rc = libusb_bulk_transfer(ml->handle, MATELIGHT_FRAMEDATA_ENDPOINT, (unsigned char*)&frame, sizeof(frame), &transferred, MATELIGHT_TIMEOUT);
			if(rc){
				fprintf(stderr, "LIBML: Cannot write frame data\n");
				return 1;
			}
			if(transferred != sizeof(frame)){
				fprintf(stderr, "LIBML: Could not transfer all frame data. Only transferred %d out of %d bytes.\n", transferred, sizeof(frame));
				return 1;
			}
		}
	}
	uint8_t payload = 0x01;
	int transferred = 0;
	int rc = libusb_bulk_transfer(ml->handle, MATELIGHT_FRAMEDATA_ENDPOINT, &payload, sizeof(uint8_t), &transferred, MATELIGHT_TIMEOUT);
	if(rc){
		fprintf(stderr, "LIBML: Cannot write frame data\n");
		return 1;
	}
	if(transferred != sizeof(uint8_t)){
		fprintf(stderr, "LIBML: Could not transfer all frame data. Only transferred %d out of %d bytes.\n", transferred, sizeof(uint8_t));
		return 1;
	}
	return 0;
}

