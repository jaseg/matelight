#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

int xterm_color_index(color_t c);

// gray
#define DEFAULT_FG_COLOR 7
// black
#define DEFAULT_BG_COLOR 0

extern color_t colortable[256];

#endif//__COLOR_H__
