.POSIX:
CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -O3 -march=native -fopenmp -ggdb3
LDFLAGS =
LDLIBS  =

race64: race64.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ race64.c $(LDLIBS)

clean:
	rm -f race64
