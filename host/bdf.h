#ifndef __MAIN_H__
#define __MAIN_H__

#include "color.h"
#include "font.h"

int framebuffer_get_text_bounds(char *s, glyphtable_t *glyph_table, size_t *outw, size_t *outh);
int framebuffer_render_text(char *s, glyphtable_t *glyph_table, color_t *gbuf, size_t gbufwidth, size_t gbufheight, size_t offx);
void console_render_buffer(color_t *data, size_t w, size_t h);

#endif//__MAIN_H__
