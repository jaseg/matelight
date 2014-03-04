#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>
#include <stdlib.h>

/* For easier memsetting we use an inverted alpha channel, i.e. 0 ≘ fully opaque; 255 ≘ fully transparent */
typedef struct {
	uint8_t r, g, b, a;
} color_t;

typedef struct {
	uint8_t r, g, b;
} rgb_t;

typedef struct {
	color_t *data;
	size_t w;
	size_t h;
} framebuffer_t;

int xterm_color_index(color_t c);

/* gray */
#define DEFAULT_FG_COLOR 7
/* black */
#define DEFAULT_BG_COLOR 0

extern color_t colortable[256];
static inline void framebuffer_free(framebuffer_t *fb){
	if(fb)
		free(fb->data);
	free(fb);
}

#endif//__COLOR_H__
