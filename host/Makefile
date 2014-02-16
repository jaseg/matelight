
all: main.c font.c font.h color.c color.h
	gcc -shared -fPIC -std=gnu11 -Wall -lm -o libbdf.so -g -O0 main.c font.c color.c

clean:
	rm libbdf.so
