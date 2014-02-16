#ifndef __MAIN_H__
#define __MAIN_H__

#include "color.h"
#include "font.h"

framebuffer_t *framebuffer_render_text(char *s, glyphtable_t *glyph_table);
void console_render_buffer(framebuffer_t *fb);

#endif//__MAIN_H__
