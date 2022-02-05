.POSIX:
CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -O3 -march=native -fopenmp
LDFLAGS = -s
LDLIBS  =

race64$(EXE): race64.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ race64.c $(LDLIBS)

check: race64$(EXE)
	./test.sh

clean:
	rm -f race64$(EXE)
