CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2 `pkg-config --cflags --libs gtk+-3.0` `pkg-config  --cflags --libs cairo`


all: cpubarchart

cpubarchart: cpubarchart.c
	$(CC) cpubarchart.c -o cpubarchart $(CFLAGS)

test: cpubarchart.c
	$(CC) -g cpubarchart.c -o cpubarchart $(CFLAGS)
	gdb cpubarchart

clean:
	rm -rf *.o cpubarchart

.PHONY:  clean

