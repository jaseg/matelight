#ifndef __GIF_H__
#define __GIF_H__

#include <gif_lib.h>
#include "color.h"

typedef struct _gifAnimationState gifAnimationState_t;

gifAnimationState_t *gif_read(uint8_t *buf, size_t buflength);
framebuffer_t* framebuffer_render_gif(uint8_t *buf, size_t buflength, gifAnimationState_t **state, int *delay);
void gifAnimationState_free(gifAnimationState_t *st);

#endif//__GIF_H__
