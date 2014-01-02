
#include "font.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void render_glyph(glyph_t *g, char *buf, unsigned int bufwidth, unsigned int offx, unsigned int offy){
	unsigned int bitmap_row_width = g->width/8;
	uint8_t *bitmap = ((uint8_t *)g) + sizeof(glyph_t);
	printf("READING GLYPH FROM %016lx (BITMAP %016lx) SIZE %d ROW WIDTH %d\n", g, bitmap, sizeof(glyph_t), bitmap_row_width);
	char *p = bitmap;
	for(int i=0; i<bitmap_row_width*g->height; i++){
		printf("%02x ", *p++);
	}
	printf("\n");
	for(unsigned int y=0; y < g->height; y++){
		long int data = 0;
		for(unsigned int i=0; i<bitmap_row_width; i++){
			data <<= 8;
			data |= bitmap[y*bitmap_row_width+i];
		}
		uint8_t *p = buf + (offy+y)*bufwidth + offx;
		printf("R %02d %04lx ", y, data);
		for(unsigned int x=0; x < g->width; x++){
			if(data&1)
				printf("█");
			else
				printf(" ");
			*p++ = (data&1) ? 1 : 0;
			data >>= 1;
		}
		printf("\n");
	}
}

int read_bdf(FILE *f, glyph_t **glyph_table, unsigned int glyph_table_size){
	char* line = 0;
	size_t len = 0;

	glyph_t current_glyph = {-1, -1, -1, -1};
	unsigned int dwidth = -1;
	unsigned int encoding = -1;
	void *glyph_data = 0;

	// Clear glyph table before use
	memset(glyph_table, 0, sizeof(*glyph_table));

	for(;;){
		size_t read = getline(&line, &len, f);
		if(read == -1){
			fprintf(stderr, "Could not read line from font file\n");
			goto error;
		}
		if(read < 2) // Ignore empty lines
			continue;
		line[read-1] = '\0';
		char *endptr;
		char *args = strchr(line, ' ');
		if(args){
			*args = 0;
			args++;
		}

		if(strcmp("ENDFONT", line) == 0){
			break;
		}else if(strcmp("ENCODING", line) == 0){
			if(!args){
				fprintf(stderr, "Invalid ENCODING line: %s %s\n", line, args);
				goto error;
			}

			encoding = strtol(args, &endptr, 10);
			if(args == endptr || *endptr != '\0'){
				fprintf(stderr, "Invalid ENCODING line: %s %s\n", line, args);
				goto error;
			}
			if(encoding > glyph_table_size){
				fprintf(stderr, "Codepoint out of valid range (0-%d): %d", glyph_table_size, encoding);
				goto error;
			}

		}else if(strcmp("BBX", line) == 0){
			if(!args){
				fprintf(stderr, "Invalid ENCODING line: %s %s\n", line, args);
				goto error;
			}

			endptr = 0;
			char *w = strtok_r(args, " ", &endptr);
			char *h = strtok_r(NULL, " ", &endptr);
			char *x = strtok_r(NULL, " ", &endptr);
			char *y = strtok_r(NULL, " ", &endptr);
			if(!w || !h || !x || !y){
				fprintf(stderr, "Invalid BBX line: %s %s\n", line, args);
				goto error;
			}
			current_glyph.width = atoi(w);
			current_glyph.height = atoi(h);
			current_glyph.x = atoi(x);
			current_glyph.y = atoi(y);

		}else if(strcmp("DWIDTH", line) == 0){
			if(!args){
				fprintf(stderr, "Invalid ENCODING line: %s %s\n", line, args);
				goto error;
			}

			endptr = 0;
			char *w = strtok_r(args, " ", &endptr);
			char *h = strtok_r(NULL, " ", &endptr);
			if(!w || !h){
				fprintf(stderr, "Invalid DWIDTH line: %s %s\n", line, args);
				goto error;
			}

			dwidth = strtol(w, &endptr, 10);
			if(w == endptr || *endptr != '\0'){
				fprintf(stderr, "Invalid DWIDTH line: %s %s\n", line, args);
				goto error;
			}

		}else if(strcmp("BITMAP", line) == 0){
			unsigned int row_bytes = dwidth/8;
			glyph_data = malloc(sizeof(glyph_t) + row_bytes * current_glyph.height);
			if(!glyph_data){
				fprintf(stderr, "Cannot malloc() memory.\n");
				goto error;
			}
			uint8_t *bitmap = (uint8_t *)glyph_data + sizeof(glyph_t);

			if(current_glyph.width < 0 || dwidth < 0 || encoding < 0){
				fprintf(stderr, "Invalid glyph: ");
				if(current_glyph.width < 0)
					fprintf(stderr, "No bounding box given.");
				if(dwidth < 0)
					fprintf(stderr, "No data width given.");
				if(encoding < 0)
					fprintf(stderr, "No codepoint given.");
				fprintf(stderr, "\n");
				goto error;
			}

			if(current_glyph.width != dwidth){
				fprintf(stderr, "Invalid glyph: bounding box width != dwidth\n");
				goto error;
			}

			unsigned int i=0;
			while(1){
				read = getline(&line, &len, f);
				if(read == -1){
					fprintf(stderr, "Could not read line from font file\n");
					goto error;
				}
				if(read < 2) // Ignore empty lines
					continue;
				line[read-1] = '\0';
				
				if(strncmp("ENDCHAR", line, read) == 0)
					break;

				if(i >= current_glyph.height){
					fprintf(stderr, "Too many rows of bitmap data in glyph %d: %d > %d\n", encoding, i+1, current_glyph.height);
					goto error;
				}

				//XXX This limits the maximum character width to sizeof(long)*8 (normally 64) piccells.
				long int data = strtol(line, &endptr, 16);
				if(line == endptr || *endptr != '\0'){
					fprintf(stderr, "Invalid bitmap data in glyph %d: %s\n", encoding, line);
					goto error;
				}

				// Right-align data
				data >>= ((read-1)*4 - dwidth);
				// Copy rightmost bytes of data to destination buffer
				if(encoding == 'A')
				printf("%02d %04lx ", i, data);
				for(unsigned int j=0; j<row_bytes; j++){
					if(encoding == 'A')
					for(unsigned int bit=0; bit<8; bit++){
						if(data&(1<<bit))
							printf("█");
						else
							printf(" ");
					}
					bitmap[(i+1)*row_bytes-j-1] = data&0xFF;
					data >>= 8;
				}
				if(encoding == 'A')
					printf("\n");
				i++;
			}
			if(encoding == 'A'){
				printf("WRITING GLYPH %d TO %016lx (BITMAP %016lx) SIZE %d ROW WIDTH %d\n", encoding, glyph_data, bitmap, sizeof(glyph_t), row_bytes);
				char *p = bitmap;
				for(int i=0; i<row_bytes*current_glyph.height; i++){
					printf("%02x ", *p++);
				}
				printf("\n");
			}
			memcpy(glyph_data, &current_glyph, sizeof(glyph_t));
			glyph_table[encoding] = glyph_data;

			// Reset things for next iteration
			current_glyph.width = -1;
			current_glyph.height = -1;
			current_glyph.x = -1;
			current_glyph.y = -1;
			dwidth = -1;
			encoding = -1;
		}
	}
	return 0;
error:
	// Free malloc()ed glyphs
	free(glyph_data);
	for(unsigned int i=0; i<sizeof(glyph_table)/sizeof(glyph_t *); i++)
		free(glyph_table[i]);
	return 1;
}

