CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

SRC     = src/perms.c src/ui.c src/main.c
TESTSRC = src/perms.c tests/test_perms.c

all: permissions

permissions: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test_perms: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $(TESTSRC)

test: test_perms
	./test_perms

clean:
	rm -f permissions test_perms

.PHONY: all test clean
