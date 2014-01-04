#ifndef __GIF_H__
#define __GIF_H__

#include <gif_lib.h>
#include "color.h"

typedef struct _gifAnimationState gifAnimationState_t;

gifAnimationState_t *gif_read(uint8_t *buf, unsigned int buflength);
framebuffer_t* gif_render(uint8_t *buf, unsigned int buflength, gifAnimationState_t **state, int *delay);
void gifAnimationState_free(gifAnimationState_t *st);

#endif//__GIF_H__
