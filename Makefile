CFLAGS=-std=gnu99 -Wall -Wno-switch -Werror -g -O2
LSLINKS_LIBS=-lgumbo -lcurl
PREFIX=/usr
CC=gcc

.PHONY: all clean install uninstall

all: build/lslinks build/joinurl build/lslinks.1.gz build/joinurl.1.gz

install: all
	install build/lslinks $(PREFIX)/bin
	install build/joinurl $(PREFIX)/bin
	install build/lslinks.1.gz $(PREFIX)/share/man/man1
	install build/joinurl.1.gz $(PREFIX)/share/man/man1

uninstall:
	rm $(PREFIX)/bin/lslinks
	rm $(PREFIX)/bin/joinurl
	rm $(PREFIX)/share/man/man1/lslinks.1.gz
	rm $(PREFIX)/share/man/man1/joinurl.1.gz

build/lslinks: build/main.o build/lslinks.o build/url.o build/bytes.o
	$(CC) $(CFLAGS) $(LSLINKS_LIBS) -o $@ build/main.o build/lslinks.o build/url.o build/bytes.o

build/joinurl: build/joinurl.o build/url.o build/bytes.o
	$(CC) $(CFLAGS) -o $@ build/joinurl.o build/url.o build/bytes.o

build/csstok: build/csstok.o build/css_tokenizer.o build/bytes.o build/unicode.o
	$(CC) $(CFLAGS) -o $@ build/csstok.o build/css_tokenizer.o build/bytes.o build/unicode.o

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

build/csstok.o: src/csstok.c src/css_tokenizer.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/unicode.o: src/unicode.c src/unicode.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/css_tokenizer.o: src/css_tokenizer.c src/css_tokenizer.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/lslinks.1.gz: man/lslinks.man
	gzip $< -c > $@

build/joinurl.1.gz: man/joinurl.man
	gzip $< -c > $@

clean:
	rm build/lslinks build/joinurl build/joinurl.o build/url.o build/bytes.o build/main.o build/lslinks.o
