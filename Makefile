CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2 -no-pie -fno-pie `pkg-config --cflags --libs gtk+-3.0` 


all: cpubarchart

cpubarchart: cpubarchart.c
	$(CC) cpubarchart.c -o cpubarchart $(CFLAGS)

test: cpubarchart.c
	$(CC) -g cpubarchart.c -o cpubarchart $(CFLAGS)
	gdb cpubarchart

clean:
	rm -rf *.o cpubarchart

.PHONY:  clean test

