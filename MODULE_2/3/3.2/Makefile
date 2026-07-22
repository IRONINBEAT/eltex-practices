CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

SRC     = src/ip4.c src/main.c
TESTSRC = src/ip4.c tests/test_ip4.c

all: routesim

routesim: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test_ip4: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $(TESTSRC)

test: test_ip4
	./test_ip4

clean:
	rm -f routesim test_ip4

.PHONY: all test clean
