
all: main.c font.c font.h color.c color.h gif.h gif.c
	gcc -shared -std=gnu11 -Wall -lm -o libbdf.so -g -O0 main.c font.c color.c

clean:
	rm libbdf.so
