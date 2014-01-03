
#include "font.h"
#include "color.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <sys/timeb.h>
#include <unistd.h>

/* CAUTION: REQUIRES INPUT TO BE \0-TERMINATED */
int console_render(char *s, glyph_t **glyph_table, unsigned int glyph_table_size){
	unsigned int len = strlen(s);

	uint8_t *gbuf = NULL;
	unsigned int gbufwidth = 0;
	unsigned int gbufheight = 0;
	char *p = s;

	wchar_t c;
	mbstate_t ps = {0};
	memset(&ps, 0, sizeof(mbstate_t));
	if(!setlocale(LC_CTYPE, "en_US.utf8")){
		fprintf(stderr, "Cannot set locale\n");
		goto error;
	}
	for(;;){
		if(*p == '\033'){
			p++;
			// Jump over escape sequences
			for(;;p++){
				if(!(*p == ';' || *p == '[' || ('0' < *p && *p < '9'))){
					p++;
					break;
				}
			}
		}

		size_t inc = mbrtowc(&c, p, MB_CUR_MAX, &ps); // MB_CUR_MAX is safe since p is \0-terminated
		if(inc == -1 || inc == -2){
			fprintf(stderr, "Error rendering string: No valid UTF-8 input.\n");
			goto error;
		}
		if(inc == 0) // Reached end of string
			break;
		p += inc;

		if(c > glyph_table_size){
			fprintf(stderr, "Error rendering string: Codepoint 0x%lx out of valid range (0-%d).\n", (long int)c, glyph_table_size);
			goto error;
		}

		glyph_t *g = glyph_table[c];
		if(!g){
			fprintf(stderr, "Error rendering string: Codepoint 0x%lx not in font.\n", (long int)c);
			goto error;
		}

		if(g->height > gbufheight)
			gbufheight = g->height;

		gbufwidth += g->width;
	}
	// For easier rendering on the terminal, round up to multiples of two
	gbufheight += gbufheight&1;

	// gbuf uses 3 bytes per color for r, g and b
	unsigned int gbufsize = gbufwidth*3*gbufheight;
	gbuf = malloc(gbufsize);
	if(gbuf == 0){
		fprintf(stderr, "Cannot malloc() %d bytes.\n", gbufsize);
		goto error;
	}
	memset(gbuf, 0, gbufsize);

	unsigned int x = 0;
	p = s;
	memset(&ps, 0, sizeof(mbstate_t));
	struct {
		color_t fg;
		color_t bg;
		unsigned int blink:4;
		unsigned int bold:1; // TODO
		unsigned int underline:1;
		unsigned int strikethrough:1;
		unsigned int fraktur:1; // TODO See: Flat10 Fraktur font
		unsigned int invert:1;
	} style = {
		colortable[DEFAULT_FG_COLOR], colortable[DEFAULT_BG_COLOR], 0, 0, 0, 0, 0, 0
	};
	for(;;){
		// NOTE: This nested escape sequence parsing does not contain any unicode-awareness whatsoever
		if(*p == '\033'){ // Escape sequence YAY
			char *sequence_start = ++p;
			if(*p == '['){ // This was a CSI!
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
				p++; // gobble up trailing 'm' of "\033[23;42m"
				nelems++;
				// By now we know it's a SGR since we error'ed out on anything else
				if(nelems < 1){
					fprintf(stderr, "Unsupported escape sequence: \"\\e%s\"\n", sequence_start);
					goto error;
				}
				for(int i=0; i<nelems; i++){
					switch(elems[i]){
						case 0: // reset style
							style.fg = colortable[DEFAULT_FG_COLOR];
							style.bg = colortable[DEFAULT_BG_COLOR];
							style.bold = 0;
							style.underline = 0;
							style.blink = 0;
							style.strikethrough = 0;
							style.fraktur = 0;
							style.invert = 0;
							break;
						case 1: // bold
							style.bold = 1;
							break;
						case 4: // underline
							style.underline = 1;
							break;
						case 5: // slow blink
							style.blink = 1;
							break;
						case 6: // rapid blink
							style.blink = 8;
							break;
						case 7: // color invert on
							style.invert = 1;
							break;
						case 9: // strike-through
							style.strikethrough = 1;
							break;
						case 20:// Fraktur
							style.fraktur = 1;
							break;
						case 22:// Bold off
							style.bold = 0;
							break;
						case 24:// Underline off
							style.underline = 0;
							break;
						case 25:// Blink off
							style.blink = 0;
							break;
						case 27:// color invert off
							style.invert = 0;
							break;
						case 29:// strike-through off
							style.strikethrough = 0;
							break;
						case 30: // Set foreground color, "dim" colors
						case 31:
						case 32:
						case 33:
						case 34:
						case 35:
						case 36:
						case 37:
							style.fg = colortable[elems[i]-30];
							break;
						case 38: // Set xterm-256 foreground color
							i++;
							if(nelems-i < 2 || elems[i] != 5){
								fprintf(stderr, "Invalid ANSI escape code: \"\\e%s\"\n", sequence_start);
								goto error;
							}
							style.fg = colortable[elems[++i]];
							break;
						case 39: // Reset foreground color to default
							style.bg = colortable[DEFAULT_FG_COLOR];
							break;
						case 40: // Set background color, "dim" colors
						case 41:
						case 42:
						case 43:
						case 44:
						case 45:
						case 46:
						case 47:
							style.bg = colortable[elems[i]-40];
							break;
						case 48: // Set xterm-256 background color
							i++;
							if(nelems-i < 2 || elems[i] != 5){
								fprintf(stderr, "Invalid ANSI escape code: \"\\e%s\"\n", sequence_start);
								goto error;
							}
							style.bg = colortable[elems[++i]];
							break;
						case 49: // Reset background color to default
							style.bg = colortable[DEFAULT_BG_COLOR];
							break;
						case 90: // Set foreground color, "bright" colors
						case 91:
						case 92:
						case 93:
						case 94:
						case 95:
						case 96:
						case 97:
							style.fg = colortable[elems[i]-90+8];
							break;
						case 100: // Set background color, "bright" colors
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
		// If p contained
		if(inc == 0) // Reached end of string
			break;
		p += inc;

		struct timeb time = {0};
		ftime(&time);
		unsigned long int t = time.time*1000 + time.millitm;
		int blink = style.blink && (t % (1000/style.blink) < (333/style.blink));
		int inv = !(style.invert ^ blink);
		color_t fg = inv ? style.fg : style.bg;
		color_t bg = inv ? style.bg : style.fg;

		glyph_t *g = glyph_table[c];
		render_glyph(g, gbuf, gbufwidth, x, 0, fg, bg);
		if(style.strikethrough || style.underline){
			int sty = gbufheight/2;
			// g->y usually is a negative index of the glyph's baseline measured from the glyph's bottom
			int uly = gbufheight + g->y;
			for(int i=0; i<g->width; i++){
				if(style.strikethrough)
					*((color_t *)(gbuf + (sty*gbufwidth + x + i)*3)) = fg;
				if(style.underline)
					*((color_t *)(gbuf + (uly*gbufwidth + x + i)*3)) = fg;
			}
		}
		x += g->width;
	}

	color_t lastfg = {0, 0, 0}, lastbg = {0, 0, 0};
	printf("\e[38;5;0;48;5;0m");
	for(unsigned int y=0; y < gbufheight; y+=2){
		for(unsigned int x=0; x < gbufwidth; x++){
			//Da magicks: ▀█▄
			color_t ct = *((color_t *)(gbuf + (y*gbufwidth + x)*3)); // Top pixel
			color_t cb = *((color_t *)(gbuf + ((y+1)*gbufwidth + x)*3)); // Bottom pixel
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
			}else{ // No matches for the upper pixel
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
		printf("\n");
	}
	return 0;
error:
	free(gbuf);
	return 1;
}

int main(int argc, char **argv){
	FILE *f = fopen("unifont.bdf", "r");
	if(!f){
		fprintf(stderr, "Error opening font file: %s\n", strerror(errno));
		return 1;
	}
	glyph_t* glyph_table[BLP_SIZE];
	if(read_bdf(f, glyph_table, BLP_SIZE)){
		fprintf(stderr, "Error reading font file.\n");
		return 1;
	}
	for(;;){
		printf("\033[2J");
		for(unsigned int i=1; i<argc; i++){
			if(console_render(argv[i], glyph_table, BLP_SIZE)){
				fprintf(stderr, "Error rendering text.\n");
				return 1;
			}
			printf("\n");
		}
		usleep(20000);
	}
	return 0;
}
