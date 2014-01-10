
#include "gif.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	uint8_t *data;
	unsigned int index;
	unsigned int length;
} readBuffer_t;	

int gif_buffer_input (GifFileType *gif, GifByteType *dest, int n){
	readBuffer_t *rb = gif->UserData;
	if(rb->index+n > rb->length)
		n = rb->length - rb->index;
	memcpy(dest, rb->data, n);
	return n;
}

struct _gifAnimationState {
	int idx;
	GifFileType *gif;
	framebuffer_t *frame;
};

void color_fill(color_t *dest, color_t src, size_t size){
	/* Some magic to reduce the amount of calls to small-block memcpys */
	dest[0] = src;
	size_t i = 1;
	size_t j = 2;
	while(j < size){
		memcpy(dest+i, dest, i*sizeof(color_t));
		i = j;
		j *= 2;
	}
	memcpy(dest+i, dest, (size-i)*sizeof(color_t));
}

gifAnimationState_t *gif_read(uint8_t *buf, size_t buflength){
	gifAnimationState_t *st = malloc(sizeof(gifAnimationState_t));
	if(!st){
		fprintf(stderr, "Failed to allocate %lu bytes\n", sizeof(*st));
		return 0;
	}

	readBuffer_t readBuf = {buf, 0, buflength};
	int err = 0;
	GifFileType *gif = DGifOpen(&readBuf, gif_buffer_input, &err);
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
		fprintf(stderr, "Failed to allocate framebuffer for GIF (%lu bytes)\n", framesize*sizeof(color_t));
		goto error;
	}
	if(gif->SColorMap){ /* Initially fill framebuffer with background color */
		GifColorType *col = gif->SColorMap[gif->SBackGroundColor].Colors;
		color_t c = {col->Red, col->Green, col->Blue, 255};
		color_fill(fb, c, framesize);
	}else{
		/* Set all pixels to a transparent black */
		memset(fb, 0, framesize*sizeof(color_t));
	}

	st->gif = gif;
	st->idx = 0;
	st->frame = malloc(sizeof(framebuffer_t));
	if(!st->frame){
		fprintf(stderr, "Failed to allocate %lu bytes\n", sizeof(framebuffer_t));
		goto error;
	}
	st->frame->data = fb;
	st->frame->w = gif->SWidth;
	st->frame->h = gif->SHeight;
	return st;
error:
	free(fb);
	free(st);
	return 0;
}

void gifAnimationState_free(gifAnimationState_t *st){
	if(st)
		framebuffer_free(st->frame);
	free(st);
}

/* buf ⇜ input data
 * state ⇜ internal state, initialize as NULL
 * delay ⇜ is filled with this frame's delay value in milliseconds (if it is not NULL). -1 means "No delay given".
 * Small note: I mean, rendering a GIF in python is not quite trivial, either (about 20loc). But come on, this is just ridiculous. */
framebuffer_t *framebuffer_render_gif(uint8_t *buf, size_t buflength, gifAnimationState_t **state, int *delay){
	gifAnimationState_t *st;
	if(*state == NULL){
		/* On first invocation, parse gif and store it in the state struct */
		st = gif_read(buf, buflength);
		if(!st)
			goto error;
		*state = st;
	}else{
		st = *state;
	}

	GifFileType *gif = st->gif;

	/* Find this image's color map */
	SavedImage *img = gif->SavedImages + st->idx;
	ColorMapObject *cmo = img->ImageDesc.ColorMap;
	if(!cmo){
		cmo = gif->SColorMap;
		if(!cmo){
			fprintf(stderr, "Missing color table for GIF frame %d\n", st->idx);
			goto error;
		}
	}

	/* Extract and validate image's bounds*/
	unsigned int ix = img->ImageDesc.Left;
	unsigned int iy = img->ImageDesc.Top;
	unsigned int iw = img->ImageDesc.Width;
	unsigned int ih = img->ImageDesc.Height;
	if((iy+ih)*gif->SWidth + ix+iw > st->frame->w*st->frame->h){
		fprintf(stderr, "Invalid gif: Image %d (x %d y %d w %d h %d) out of bounds (w %d h %d)\n",
				st->idx, ix, iy, iw, ih, gif->SWidth, gif->SHeight);
		goto error;
	}

	/* Find and parse this image's GCB (if it exists) */
	int transparent = -1;
	int disposal = DISPOSAL_UNSPECIFIED;
	if(delay)
		*delay = -1; /* in milliseconds */
	for(unsigned int i=0; i<img->ExtensionBlockCount; i++){
		ExtensionBlock *eb = img->ExtensionBlocks + i;
		if(eb->Function == GRAPHICS_EXT_FUNC_CODE){
			GraphicsControlBlock *gcb = (GraphicsControlBlock *)eb->Bytes;
			transparent = gcb->TransparentColor;
			disposal = gcb->DisposalMode;
			if(delay)
				*delay = gcb->DelayTime * 10; /* 10ms → 1ms */
		}
	}

	color_t *fb = st->frame->data;
	if(disposal == DISPOSE_BACKGROUND || disposal == DISPOSAL_UNSPECIFIED){
		GifColorType *gc = gif->SColorMap[gif->SBackGroundColor].Colors;
		color_t c = {gc->Red, gc->Green, gc->Blue, 255};
		color_fill(fb, c, st->frame->w*st->frame->h);
	}
	/* FIXME DISPOSE_PREVIOUS is currently unhandled. */

	/* Render gif bitmap to RGB */
	uint8_t *p = img->RasterBits;
	for(unsigned int y = iy; y < iy+ih; y++){
		for(unsigned int x = ix; x < ix+iw; x++){
			int c = *p++;
			if(c != transparent){
				GifColorType *col = cmo[c].Colors;
				color_t ct = {col->Red, col->Green, col->Blue, 255};
				fb[y*st->frame->w + x] = ct;
			}
		}
	}

	st->idx++;
	if(st->idx >= gif->ImageCount)
		st->idx = 0;
	return st->frame;
error:
	gifAnimationState_free(st);
	return 0;
}

