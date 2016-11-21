CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

INPUTFILESIZEMEGABYTES = 1

KILO = 1024
MEGA = $(shell echo $(KILO)\*$(KILO) | bc)
INPUTFILESIZEBYTES = $(shell echo $(MEGA)\*$(INPUTFILESIZEMEGABYTES) | bc)
INPUTBLOCKSIZEBYTES = $(KILO)
INPUTBLOCKS = $(shell echo $(INPUTFILESIZEBYTES)\/$(INPUTBLOCKSIZEBYTES) | bc)

.PHONY: all clean

all: cpu-bound io-bound mixed

cpu-bound: cpu-bound.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

io-bound: io-bound.o rwinput
	$(CC) $(LFLAGS) io-bound.o -o $@ -lm

mixed: mixed.o rwinput
	$(CC) $(LFLAGS) mixed.o -o $@ -lm

cpu-bound.o: cpu-bound.c
	$(CC) $(CFLAGS) $<

io-bound.o: io-bound.c
	$(CC) $(CFLAGS) $<

mixed.o: mixed.c
	$(CC) $(CFLAGS) $<

rwinput: Makefile
	dd if=/dev/urandom of=./rwinput bs=$(INPUTBLOCKSIZEBYTES) count=$(INPUTBLOCKS)

clean: testclean
	rm -f cpu-bound io-bound mixed
	rm -f rwinput
	rm -f *.o
	rm -f *~

testclean:
	rm -f rwoutput*
