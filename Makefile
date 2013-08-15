CFLAGS=-std=gnu99 -Wall -Wno-switch -Werror -g -O2
LIBS=-lgumbo -lcurl

.PHONY: all clean

all: lslinks joinurl

lslinks: main.o lslinks.o url.o bytes.o
	gcc $(CFLAGS) $(LIBS) -o $@ main.o lslinks.o url.o bytes.o

joinurl: joinurl.o url.o
	gcc $(CFLAGS) $(LIBS) -o $@ joinurl.o url.o

lslinks.o: lslinks.c lslinks.h
	gcc $(CFLAGS) $(LIBS) -o $@ -c $<

joinurl.o: joinurl.c url.h
	gcc $(CFLAGS) $(LIBS) -o $@ -c $<

main.o: main.c lslinks.h
	gcc $(CFLAGS) $(LIBS) -o $@ -c $<

bytes.o: bytes.c bytes.h
	gcc $(CFLAGS) $(LIBS) -o $@ -c $<

url.o: url.c bytes.h
	gcc $(CFLAGS) $(LIBS) -o $@ -c $<

clean:
	rm lslinks joinurl joinurl.o url.o bytes.o main.o lslinks.o
