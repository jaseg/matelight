
#include "gif.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	uint8_t *data;
	unsigned int index;
	unsigned int length;
} readBuffer_t;	

int gifBufferInput (GifFileType *gif, GifByteType *dest, int n){
	readBuffer_t *rb = gif->userData;
	if(rb->index+n > rb->length)
		n = rb->length - rb->index;
	memcpy(dest, rb->data, n);
	return n;
}

struct _gifAnimationState {
	int idx;
	GifFileType *gif;
	framebuffer_t frame;
};

gifAnimationState_t *gif_read(uint8_t *buf, unsigned int buflength){
	/* On first invocation, parse gif and store it in the state struct */
	gifAnimationState_t *st = malloc(sizeof(*state));
	if(!st){
		fprintf(stderr, "Failed to allocate %d bytes\n", sizeof(*state));
		return 0;
	}

	readBuffer_t readBuf = {buf, 0, buflength};
	int err = 0;
	GifFileType *gif = DGifOpen(readBuf, OutputFunc writeFunc, &err);
	if(err){
		fprintf(stderr, "Could not read GIF data: %s\n", GifErrorString(err));
		goto error;
	}

	unsigned int framesize = gif->SWidth*gif->SHeight;
	if(framesize == 0){ /* Can this actually happen? */
		fprintf(stderr, "Invalid 0*0px gif\n");
		goto error;
	}
	color_t *fb = calloc(framesize, sizeof(color_t));
	if(!fb){
		fprintf(stderr, "Failed to allocate framebuffer for GIF (%d bytes)\n", gif->SWidth*gif->SHeight);
		goto error;
	}
	if(gif->SColorMap){ /* Initially fill framebuffer with background color */
		GifColorType *col = gif->SColorMap[gif->SBackGroundColor]->Colors;
		color_t c = {col->Red, col->Green, col->Blue, 255};
		/* Some magic to reduce the amount of calls to small-block memcpys */
		fb[0] = c;
		int i = 1;
		int j = 2;
		while(j < framesize){
			memcpy(fb+i*sizeof(color_t), fb, i*sizeof(color_t));
			i = j;
			j *= 2;
		}
		memcpy(fb+i*sizeof(color_t), fb, (framesize-i)*sizeof(color_t))
	}else{
		/* Set all pixels to a transparent black */
		memset(fb, 0, framesize*sizeof(color_t));
	}

	st->gif = gif;
	st->idx = 0;
	st->frame->data = fb;
	st->frame->w = gif->SWidth;
	st->frame->h = gif->SHeight;
	return st;
error:
	free(fb);
	free(st);
	return 0;
}

/* buf ⇜ input data
 * state ⇜ internal state, initialize as NULL */
framebuffer_t* gif_render(uint8_t *buf, unsigned int buflength, gifAnimationState_t **state){
	gifAnimationState_t *st;
	if(*state == NULL){
		st = gif_read(buf, buflength, state);
		if(!st)
			goto error;
		*state = st;
	}else{
		st = *state;
	}

	GifFileType *gif = st->gif;

	SavedImage *img = gif->images[st->idx];
	cmo = img->ImageDesc->ColorMap;
	if(!cmo){
		ColorMapObject *cmo = gif->SColorMap;
		if(!cmo){
			fprintf(stderr, "Missing color table for GIF frame %d\n", st->idx);
			goto error;
		}
	}
	unsigned int ix = img->ImageDesc->Left;
	unsigned int iy = img->ImageDesc->Top;
	unsigned int iw = img->ImageDesc->Width;
	unsigned int ih = img->ImageDesc->Height;
	if((iy+ih)*gif->SWidth + ix+iw > st->frame->w*st->frame->h;){
		fprintf(stderr, "Invalid gif: Image %d (x %d y %d w %d h %d) out of bounds (w %d h %d)\n",
				st->idx, ix, iy, iw, ih, gif->SWidth, gif->SHeight);
		goto error;
	}
	uint8_t *p = img->RasterBits;
	color_t *fb = st->frame->data;
	for(unsigned int y = iy; y < iy+ih; y++){
		for(unsigned int x = ix; x < ix+iw; x++){
			GifColorType col = cmo[*p++]->Colors;
			fb[y*st->frame->w + x] = {col.Red, col.Green, col.Blue, 255};
		}
	}

	st->idx++;
	if(st->idx >= gif->ImageCount)
		st->idx = 0;
	return fb;
error:
	if(st)
		free(st->frame->data);
	free(st);
	return 0;
}

