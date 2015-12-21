ROOT = ../../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/versioned-skiplist

CFLAGS +=  -std=gnu11 -Wall -pedantic
#LDFLAGS += -ltcmalloc

CC = gcc

.PHONY:	all clean

all:	main

skiplist-ver.o: skiplist-ver.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist-ver.o skiplist-ver.c

versioned.o: versioned.h skiplist-ver.h 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/versioned.o versioned.c 

# seq.o: skiplist-ver.h versioned.h
# 	$(CC) $(CFLAGS) -c -o $(BUILDIR)/seq.o seq.c

main: skiplist-ver.o versioned.o test.c 
	$(CC) $(CFLAGS) $(BUILDIR)/skiplist-ver.o $(BUILDIR)/versioned.o test.c -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS) *.o
