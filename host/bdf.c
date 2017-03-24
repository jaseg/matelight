/* Matelight
 * Copyright (C) 2016 Sebastian Götte <code@jaseg.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "bdf.h"
#include "color.h"
#include "font.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <sys/timeb.h>
#include <sys/queue.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* CAUTION: REQUIRES INPUT TO BE \0-TERMINATED
 * ...also, it does a hardcoded setlocale of LC_CTYPE to en_US.utf8 for... reasons. */
int framebuffer_get_text_bounds(char *s, glyphtable_t *glyph_table, size_t *outw, size_t *outh){
	size_t gbufwidth = 0;
	size_t gbufheight = 0;
	char *p = s;

	/* Calculate screen width of string prior to allocating memory for the frame buffer */
	wchar_t c;
	mbstate_t ps = {0};
	memset(&ps, 0, sizeof(mbstate_t));
	if(!setlocale(LC_CTYPE, "en_US.utf8")){
		fprintf(stderr, "Cannot set locale\n");
		goto error;
	}
	for(;;){
		while(*p == '\033'){
			p++;
			/* Jump over escape sequences */
			for(;;p++){
				if(!(*p == ';' || *p == '[' || ('0' <= *p && *p <= '9'))){
					p++;
					break;
				}
			}
			memset(&ps, 0, sizeof(mbstate_t));
		}

		size_t inc = mbrtowc(&c, p, MB_CUR_MAX, &ps); /* MB_CUR_MAX is safe since p is \0-terminated */
		if(inc == -1 || inc == -2){
			fprintf(stderr, "Error rendering string: No valid UTF-8 input.\n");
			goto error;
		}
		if(inc == 0) /* Reached end of string */
			break;
		p += inc;

		if(c > glyph_table->size){
			fprintf(stderr, "Error rendering string: Codepoint 0x%lx out of valid range (0-%zd).\n", (long int)c, glyph_table->size);
			goto error;
		}

		glyph_t *g = glyph_table->data[c];
		if(!g){
			fprintf(stderr, "Error rendering string: Codepoint 0x%lx not in font.\n", (long int)c);
			goto error;
		}

		if(g->height > gbufheight)
			gbufheight = g->height;


		gbufwidth += g->width;
	}
	*outw = gbufwidth;
	*outh = gbufheight;
	return 0;
error:
	return 1;
}

/* CAUTION: REQUIRES INPUT TO BE \0-TERMINATED
 * ...also, it does a hardcoded setlocale of LC_CTYPE to en_US.utf8 for... reasons. */
/* Render the string beginning from the specified x offset (in pixels) */
int framebuffer_render_text(char *s, glyphtable_t *glyph_table, color_t *gbuf, size_t gbufwidth, size_t gbufheight, size_t offx){
	unsigned int len = strlen(s);
	char *p = s;

	if(!setlocale(LC_CTYPE, "en_US.utf8")){
		fprintf(stderr, "Cannot set locale\n");
		goto error;
	}

	memset(gbuf, 0, gbufwidth*gbufheight*sizeof(color_t));

	unsigned int x = 0;
	wchar_t c;
	p = s;
	mbstate_t ps = {0};
	memset(&ps, 0, sizeof(mbstate_t));
	struct {
		color_t fg;
		color_t bg;
		unsigned int blink:4;
		unsigned int bold:1; /* TODO */
		unsigned int underline:1;
		unsigned int strikethrough:1;
		unsigned int fraktur:1; /* TODO See: Flat10 Fraktur font */
		unsigned int invert:1;
	} style = {
		colortable[DEFAULT_FG_COLOR], colortable[DEFAULT_BG_COLOR], 0, 0, 0, 0, 0, 0
	};
	/* Render glyphs (now with escape sequence rendering!) */
	for(;;){
		/* NOTE: This nested escape sequence parsing does not contain any unicode-awareness whatsoever */
		if(*p == '\033'){ /* Escape sequence YAY */
			char *sequence_start = ++p;
			if(*p == '['){ /* This was a CSI! */
				/* Disassemble the list of numbers, only accept SGR sequences (those ending with 'm') */
				long elems[MAX_CSI_ELEMENTS];
				int nelems;
				for(nelems = 0; nelems<MAX_CSI_ELEMENTS; nelems++){
					p++;
					char *endptr;
					elems[nelems] = strtol(p, &endptr, 10);
					if(p == endptr){
						fprintf(stderr, "Invalid escape sequence: \"\\e%s\"\n", sequence_start);
						goto error;
					}
					p = endptr;
					if(*endptr == 'm')
						break;
					if(*endptr != ';'){
						fprintf(stderr, "Invalid escape sequence: \"\\e%s\"\n", sequence_start);
						goto error;
					}
				}
				p++; /* gobble up trailing 'm' of "\033[23;42m" */
				nelems++;
				/* By now we know it's a SGR since we error'ed out on anything else */
				if(nelems < 1){
					fprintf(stderr, "Unsupported escape sequence: \"\\e%s\"\n", sequence_start);
					goto error;
				}
				/* Parse the sequence numbers */
				for(int i=0; i<nelems; i++){
					switch(elems[i]){
						case 0: /* reset style */
							style.fg = colortable[DEFAULT_FG_COLOR];
							style.bg = colortable[DEFAULT_BG_COLOR];
							style.bold = 0;
							style.underline = 0;
							style.blink = 0;
							style.strikethrough = 0;
							style.fraktur = 0;
							style.invert = 0;
							break;
						case 1: /* bold */
							style.bold = 1;
							break;
						case 4: /* underline */
							style.underline = 1;
							break;
						case 5: /* slow blink */
							style.blink = 1;
							break;
						case 6: /* rapid blink */
							style.blink = 8;
							break;
						case 7: /* color invert on */
							style.invert = 1;
							break;
						case 9: /* strike-through */
							style.strikethrough = 1;
							break;
						case 20:/* Fraktur */
							style.fraktur = 1;
							break;
						case 22:/* Bold off */
							style.bold = 0;
							break;
						case 24:/* Underline off */
							style.underline = 0;
							break;
						case 25:/* Blink off */
							style.blink = 0;
							break;
						case 27:/* color invert off */
							style.invert = 0;
							break;
						case 29:/* strike-through off */
							style.strikethrough = 0;
							break;
						case 30: /* Set foreground color, "dim" colors */
						case 31:
						case 32:
						case 33:
						case 34:
						case 35:
						case 36:
						case 37:
							style.fg = colortable[elems[i]-30];
							break;
						case 38: /* Set xterm-256 foreground color */
							i++;
							if(nelems-i < 2 || elems[i] != 5){
								fprintf(stderr, "Invalid ANSI escape code: \"\\e%s\"\n", sequence_start);
								goto error;
							}
							style.fg = colortable[elems[++i]];
							break;
						case 39: /* Reset foreground color to default */
							style.bg = colortable[DEFAULT_FG_COLOR];
							break;
						case 40: /* Set background color, "dim" colors */
						case 41:
						case 42:
						case 43:
						case 44:
						case 45:
						case 46:
						case 47:
							style.bg = colortable[elems[i]-40];
							break;
						case 48: /* Set xterm-256 background color */
							i++;
							if(nelems-i < 2 || elems[i] != 5){
								fprintf(stderr, "Invalid ANSI escape code: \"\\e%s\"\n", sequence_start);
								goto error;
							}
							style.bg = colortable[elems[++i]];
							break;
						case 49: /* Reset background color to default */
							style.bg = colortable[DEFAULT_BG_COLOR];
							break;
						case 90: /* Set foreground color, "bright" colors */
						case 91:
						case 92:
						case 93:
						case 94:
						case 95:
						case 96:
						case 97:
							style.fg = colortable[elems[i]-90+8];
							break;
						case 100: /* Set background color, "bright" colors */
						case 101:
						case 102:
						case 103:
						case 104:
						case 105:
						case 106:
						case 107:
							style.bg = colortable[elems[i]-100+8];
							break;
						default:
							fprintf(stderr, "Unsupported escape sequence: \"\\e%s\"\n", sequence_start);
							goto error;
					}
				}

			}else{
				fprintf(stderr, "Unsupported escape sequence: \"\\e%s\"\n", sequence_start);
				goto error;
			}

			continue;
		}

		size_t inc = mbrtowc(&c, p, (s+len+1)-p, NULL);
		/* If p contained */
		if(inc == 0) /* Reached end of string */
			break;
		p += inc;

		/* Render glyph into frame buffer */
		struct timeb time = {0};
		ftime(&time);
		unsigned long int t = time.time*1000 + time.millitm;
		int blink = style.blink && (t % (1000/style.blink) < (333/style.blink));
		int inv = !(style.invert ^ blink);
		color_t fg = inv ? style.fg : style.bg;
		color_t bg = inv ? style.bg : style.fg;

		glyph_t *g = glyph_table->data[c];
		/* Is the glyph within the buffer's bounds? */
		//if(x+g->width > offx && x < offx+gbufwidth){
			/* x-offx might be negative down to -g->width+1, but that's ok */
			render_glyph(g, gbuf, gbufwidth, x-offx, 0, fg, bg);
			if(style.strikethrough || style.underline){
				int sty = gbufheight/2;
				/* g->y usually is a negative index of the glyph's baseline measured from the glyph's bottom */
				int uly = gbufheight + g->y;
				for(int i=0; i<g->width; i++){
					if(x+i >= offx && x+i-offx < gbufwidth){ /* Stay within the frame buffer's bounds */
						if(style.strikethrough)
							gbuf[sty*gbufwidth + x + i - offx] = fg;
						if(style.underline)
							gbuf[uly*gbufwidth + x + i - offx] = fg;
					}
				}
			}
		//}
		x += g->width;
	}
	return 0;
error:
	return 1;
}

void console_render_buffer(color_t *data, size_t w, size_t h){
	/* Render framebuffer to terminal, two pixels per character using Unicode box drawing stuff */
	color_t lastfg = {0, 0, 0}, lastbg = {0, 0, 0};
	printf("\e[38;5;0;48;5;0m\e[K");
	for(size_t y=0; y < h; y+=2){
		for(size_t x=0; x < w; x++){
			/* Da magicks: ▀█▄ */
			color_t ct = data[y*w + x]; /* Top pixel */
			color_t cb = data[(y+1)*w + x]; /* Bottom pixel */
			/* The following, rather convoluted logic tries to "save" escape sequences when rendering. */
			if(!memcmp(&ct, &lastfg, sizeof(color_t))){
				if(!memcmp(&cb, &lastbg, sizeof(color_t))){
					printf("▀");
				}else if(!memcmp(&cb, &lastfg, sizeof(color_t))){
					printf("█");
				}else{
					printf("\033[48;5;%dm▀", xterm_color_index(cb));
					lastbg = cb;
				}
			}else if(!memcmp(&ct, &lastbg, sizeof(color_t))){
				if(!memcmp(&cb, &lastfg, sizeof(color_t))){
					printf("▄");
				}else if(!memcmp(&cb, &lastbg, sizeof(color_t))){
					printf(" ");
				}else{
					printf("\033[38;5;%dm▄", xterm_color_index(cb));
					lastfg = cb;
				}
			}else{ /* No matches for the upper pixel */
				if(!memcmp(&cb, &lastfg, sizeof(color_t))){
					printf("\033[48;5;%dm▄", xterm_color_index(ct));
					lastbg = ct;
				}else if(!memcmp(&cb, &lastbg, sizeof(color_t))){
					printf("\033[38;5;%dm▀", xterm_color_index(ct));
					lastfg = ct;
				}else{
					printf("\033[38;5;%d;48;5;%dm▀", xterm_color_index(ct), xterm_color_index(cb));
					lastfg = ct;
					lastbg = cb;
				}
			}
		}
		printf("\n\e[K");
	}
	printf("\033[0m");
}

