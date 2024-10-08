all:
	gcc -Wall -O3 -o lowkey lowkey.c -lX11 -lpng -lasound
clean:
	rm -f lowkey