CC=gcc
DEFINES=-DMALLOC_DEBUG -DMALLOC_DETECT_DOUBLE_FREE
CFLAGS=-g -Wall
LDFLAGS=-ldl

driver: driver.o malloc.o
	$(CC) $(CFLAGS) $(DEFINES) -o driver driver.o malloc.o $(LDFLAGS)

driver.o: driver.c
	$(CC) $(CFLAGS) $(DEFINES) -c driver.c

malloc.o: malloc.c malloc.h list.h
	$(CC) $(CFLAGS) $(DEFINES) -c malloc.c

clean:
	rm -f driver
	rm -f driver.o
	rm -f malloc.o

