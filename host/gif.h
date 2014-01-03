#ifndef __GIF_H__
#define __GIF_H__

#include <gif_lib.h>

typedef struct _gifAnimationState gifAnimationState_t;

color_t* gif_render(uint8_t *buf, unsigned int buflength, gifAnimationState_t **state);

#endif//__GIF_H__
