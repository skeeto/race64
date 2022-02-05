.POSIX:
CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -O3 -march=native -fopenmp
LDFLAGS = -s
LDLIBS  =
PREFIX  = /usr/local

race64$(EXE): race64.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ race64.c $(LDLIBS)

check: race64$(EXE)
	./test.sh

install: race64$(EXE)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 755 race64$(EXE) $(DESTDIR)$(PREFIX)/bin
	gzip <race64.1 >$(DESTDIR)$(PREFIX)/share/man/man1/race64.1.gz

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/race64$(EXE)
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/race64.1.gz

clean:
	rm -f race64$(EXE)
