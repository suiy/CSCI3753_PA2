CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall -Wextra -pthread

.PHONY: all clean

all: test multi-lookup lookup queueTest pthread-hello

test: test.o queue.o util.o
	$(CC) $(LFLAGS) $^ -o $@

multi-lookup: multi-lookup.o queue.o util.o
	$(CC) $(LFLAGS) $^ -o $@

lookup: lookup.o queue.o util.o
	$(CC) $(LFLAGS) $^ -o $@

queueTest: queueTest.o queue.o
	$(CC) $(LFLAGS) $^ -o $@

pthread-hello: pthread-hello.o
	$(CC) $(LFLAGS) $^ -o $@

test.o: test.c
	$(CC) $(CFLAGS) $<

multi-lookup.o: multi-lookup.c
	$(CC) $(CFLAGS) $<

lookup.o: lookup.c
	$(CC) $(CFLAGS) $<

queueTest.o: queueTest.c
	$(CC) $(CFLAGS) $<

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) $<

util.o: util.c util.h
	$(CC) $(CFLAGS) $<

pthread-hello.o: pthread-hello.c
	$(CC) $(CFLAGS) $<

clean:
	rm -f lookup queueTest pthread-hello
	rm -f *.o
	rm -f *~
	rm -f results.txt