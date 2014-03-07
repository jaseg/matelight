
#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>
#include <stdio.h>
#include "color.h"

// CAUTION: A glyph struct is always followed by the glyph's bitmap.
typedef struct {
	uint8_t width;
	uint8_t height;
	int8_t x;
	int8_t y;
} glyph_t;

typedef struct {
	glyph_t **data;
	size_t size;
} glyphtable_t;

// Size of Unicode's basic multilingual plane
#define BLP_SIZE	65536

#define MAX_CSI_ELEMENTS 8

// We could also use some fancy hashtable here, but unifont includes about 57k glyphs so we would hardly save any memory.
glyphtable_t *read_bdf(FILE *f);

void free_glyphtable(glyphtable_t *glyph_table);

void render_glyph(glyph_t *g, color_t *buf, unsigned int bufwidth, int offx, unsigned int offy, color_t fg, color_t bg);

#endif//__FONT_H__
