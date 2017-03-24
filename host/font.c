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

#include "font.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Clips at buffer bounds */
void render_glyph(glyph_t *g, color_t *buf, unsigned int bufwidth, int offx, unsigned int offy, color_t fg, color_t bg){
	unsigned int bitmap_row_width = g->width/8;
	uint8_t *bitmap = ((uint8_t *)g) + sizeof(glyph_t);
	for(unsigned int y=0; y < g->height; y++){
		long int data = 0;
		for(unsigned int i=0; i<bitmap_row_width; i++){
			data <<= 8;
			data |= bitmap[y*bitmap_row_width+i];
		}
		color_t *p = buf + (offy+y)*bufwidth + offx;
		/* Take care to only render what's within the framebuffer's bounds */
		for(int x=0; (x < g->width) && (offx+x < (int)bufwidth); x++){
			if(offx + x >= 0){
				color_t c = (data&(1<<(g->width-1))) ? fg : bg;
				*p = c;
			}
			p++;
			data <<= 1;
		}
	}
}

glyphtable_t *read_bdf_file(char *filename){
	FILE *fontfile = fopen("unifont.bdf", "r");
	if(!fontfile){
		fprintf(stderr, "Error opening font file: %s\n", strerror(errno));
		goto error;
	}

	glyphtable_t *glyph_table = read_bdf(fontfile);
	if(!glyph_table){
		fprintf(stderr, "Error reading font file.\n");
		goto error;
	}

	fclose(fontfile);
	return glyph_table;
error:
	fclose(fontfile);
	return NULL;
}

#define START_GLYPHTABLE_SIZE 256
#define MAX_GLYPHTABLE_INCREMENT 32768
glyphtable_t *extend_glyphtable(glyphtable_t *glyph_table){
	size_t oldlen = glyph_table->size;
	size_t newlen = oldlen + (oldlen<MAX_GLYPHTABLE_INCREMENT ? oldlen : MAX_GLYPHTABLE_INCREMENT);
	if(oldlen == 0)
		newlen = START_GLYPHTABLE_SIZE;
	glyph_t **newdata = realloc(glyph_table->data, newlen*sizeof(glyph_t*));
	if(!newdata){
		fprintf(stderr, "Cannot allocate bdf glyph buffer\n");
		goto error;
	}
	glyph_table->data = newdata;
	// Clear newly allocated memory area
	memset(glyph_table->data+oldlen, 0, (newlen-oldlen)*sizeof(glyph_t*));
	glyph_table->size = newlen;
	return glyph_table;
error:
	free_glyphtable(glyph_table);
	return NULL;
}

void free_glyphtable(glyphtable_t *glyph_table){
	if(glyph_table){
		for(unsigned int i=0; i<glyph_table->size; i++){
			free(glyph_table->data[i]);
		}
		free(glyph_table->data);
	}
	free(glyph_table);
}

glyphtable_t *read_bdf(FILE *f){
	glyphtable_t *glyph_table = malloc(sizeof(glyphtable_t));
	if(!glyph_table){
		fprintf(stderr, "Cannot allocate glyph table\n");
		goto error;
	}
	memset(glyph_table, 0, sizeof(*glyph_table));

	char* line = 0;
	size_t len = 0;

	glyph_t current_glyph = {-1, -1, -1, -1};
	unsigned int dwidth = -1;
	unsigned int encoding = -1;
	void *glyph_data = 0;

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
				for(unsigned int j=0; j<row_bytes; j++){
					bitmap[(i+1)*row_bytes-j-1] = data&0xFF;
					data >>= 8;
				}
				i++;
			}

			if(encoding >= glyph_table->size){
				glyph_table = extend_glyphtable(glyph_table);
				if(!glyph_table){
					fprintf(stderr, "Cannot extend glyph table.\n");
					goto error;
				}
			}

			memcpy(glyph_data, &current_glyph, sizeof(glyph_t));
			glyph_table->data[encoding] = glyph_data;

			// Reset things for next iteration
			current_glyph.width = -1;
			current_glyph.height = -1;
			current_glyph.x = -1;
			current_glyph.y = -1;
			dwidth = -1;
			encoding = -1;
		}
	}
	return glyph_table;
error:
	free(glyph_data);
	free_glyphtable(glyph_table);
	return NULL;
}

