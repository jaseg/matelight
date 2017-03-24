/* Matelight
 * Copyright (C) 2016 Sebastian Götte <code@jaseg.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

matelight_handle *matelight_open(char *match_serial){
	matelight_handle *out = NULL;
    libusb_device_handle *handle = NULL;
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
		out = calloc(1, sizeof(matelight_handle));
		if(!out){
			fprintf(stderr, "LIBML: Cannot allocate memory\n");
			goto error;
		}
		for(ssize_t i=0; i<res; i++){
			libusb_get_device_descriptor(device_list[i], &desc);
			if(desc.idVendor == MATELIGHT_VID && desc.idProduct == MATELIGHT_PID){
				if(libusb_open(device_list[i], &handle)){
					fprintf(stderr, "LIBML: Cannot open Mate Light USB device\n");
					goto error;
				}
				out->handle = handle;
				if(matelight_cmp_str_desc(handle, desc.iManufacturer, "Gold & Apple"))
				if(matelight_cmp_str_desc(handle, desc.iProduct, "Mate Light"))
				if(!match_serial || matelight_cmp_str_desc(handle, desc.iSerialNumber, match_serial)){
#define BUF_SIZE 256
					out->serial = malloc(BUF_SIZE);
					if(!out->serial){ fprintf(stderr, "LIBML: Cannot allocate memory\n");
						goto error;
					}
					if(libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, (unsigned char*)&out->serial, BUF_SIZE) < 0){
						fprintf(stderr, "LIBML: Cannot read device string descriptor\n");
						goto error;
					}
#undef BUF_SIZE
                    return out;
				}
			}
		}
	}
	libusb_free_device_list(device_list, 1);
	return NULL;
error:
	if(out){
        if(out->serial)
            free(out->serial);
        free(out);
    }
    if(handle)
        libusb_close(handle);
	libusb_free_device_list(device_list, 1);
	return NULL;
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
					color_t *src;
					if(alpha){
						src = (((color_t*)buf)+spos);
					}else{
						src = (color_t*)(((rgb_t*)buf)+spos);
					}
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
				fprintf(stderr, "LIBML: Could not transfer all frame data. Only transferred %d out of %zu bytes.\n", transferred, sizeof(frame));
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
		fprintf(stderr, "LIBML: Could not transfer all frame data. Only transferred %d out of %zu bytes.\n", transferred, sizeof(uint8_t));
		return 1;
	}
	return 0;
}

