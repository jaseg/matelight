
#include "font.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>

/* CAUTION: REQUIRES INPUT TO BE \0-TERMINATED */
int console_render(char *s, glyph_t **glyph_table, unsigned int glyph_table_size){
	unsigned int len = strlen(s);
	char *t = s;
	wchar_t *buf = calloc(len, sizeof(wchar_t));
	if(buf == 0){
		fprintf(stderr, "Cannot calloc() %ld bytes.\n", len*sizeof(wchar_t));
		goto error;
	}
	mbsrtowcs(buf, &t, len, NULL);

	char *gbuf = NULL;
	unsigned int gbufwidth = 0;
	unsigned int gbufheight = 0;
	for(wchar_t *c=buf; *c; c++){
		if(*c > glyph_table_size){
			fprintf(stderr, "Error rendering string: Codepoint 0x%lx out of valid range (0-%d).\n", (long int)c, glyph_table_size);
			goto error;
		}

		glyph_t *g = glyph_table[*c];
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

	unsigned int gbufsize = gbufwidth*gbufheight;
	gbuf = malloc(gbufsize);
	if(gbuf == 0){
		fprintf(stderr, "Cannot malloc() %d bytes.\n", gbufsize);
		goto error;
	}

	unsigned int x = 0;
	for(wchar_t *c=buf; *c; c++){
		glyph_t *g = glyph_table[*c];
		render_glyph(g, gbuf, gbufwidth, x, 0);
		x += g->width;
	}

	for(unsigned int y=0; y < gbufheight; y++){
		for(unsigned int x=0; x < gbufwidth; x++){
			//Da magicks: ▀█▄
			if(gbuf[y*gbufwidth + x])
				printf("█");
			else
				printf(" ");
			/*
			char c1 = gbuf[y*gbufwidth + x];
			char c2 = gbuf[(y+1)*gbufwidth + x];
			if(c1 && c2)
				printf("█");
			else if(c1 && !c2)
				printf("▀");
			else if(!c1 && c2)
				printf("▄");
			else
				printf(" ");
			*/
		}
		printf("\n");
	}
	return 0;
error:
	free(gbuf);
	free(buf);
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
	for(unsigned int i=1; i<argc; i++){
		if(console_render(argv[i], glyph_table, BLP_SIZE)){
			fprintf(stderr, "Error rendering text.\n");
			return 1;
		}
		printf("\n");
	}
	return 0;
}
