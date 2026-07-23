CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

SRC     = src/pqueue.c src/ui.c src/main.c
TESTSRC = src/pqueue.c tests/test_pqueue.c

all: pqueue

pqueue: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test_pqueue: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $(TESTSRC)

test: test_pqueue
	./test_pqueue

clean:
	rm -f pqueue test_pqueue

.PHONY: all test clean
