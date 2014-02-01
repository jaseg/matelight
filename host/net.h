#ifndef __NET_H__
#define __NET_H__

#include "config.h"

typedef struct {
	uint8_t r, g, b;
} rgb_t;

typedef struct {
	uint32_t magic, seq;
	uint16_t width, height;
	rgb_t data[DISPLAY_WIDTH*DISPLAY_HEIGHT];
} ml_packet_t;

#define UDP_BUF_SIZE	sizeof(ml_packet_t)

#endif//__NET_H__
