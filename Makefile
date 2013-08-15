CFLAGS=-std=gnu99 -Wall -Wno-switch -Werror -g -O2
LSLINKS_LIBS=-lgumbo -lcurl
JOINURL_LIBS=-lgumbo
PREFIX=/usr
CC=gcc

.PHONY: all clean install uninstall

all: build/lslinks build/joinurl

install: all
	install build/lslinks $(PREFIX)/bin
	install build/joinurl $(PREFIX)/bin

uninstall:
	rm $(PREFIX)/bin/lslinks
	rm $(PREFIX)/bin/joinurl

build/lslinks: build/main.o build/lslinks.o build/url.o build/bytes.o
	$(CC) $(CFLAGS) $(LSLINKS_LIBS) -o $@ build/main.o build/lslinks.o build/url.o build/bytes.o

build/joinurl: build/joinurl.o build/url.o
	$(CC) $(CFLAGS) $(JOINURL_LIBS) -o $@ build/joinurl.o build/url.o

build/lslinks.o: src/lslinks.c src/lslinks.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/joinurl.o: src/joinurl.c src/url.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/main.o: src/main.c src/lslinks.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/bytes.o: src/bytes.c src/bytes.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/url.o: src/url.c src/bytes.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm build/lslinks build/joinurl build/joinurl.o build/url.o build/bytes.o build/main.o build/lslinks.o
