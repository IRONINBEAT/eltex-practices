CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

SRC     = src/contact.c src/utf8.c src/phonebook.c src/storage.c \
          src/ui.c src/main.c
TESTSRC = src/contact.c src/utf8.c src/phonebook.c src/storage.c \
          tests/test_phonebook.c

all: phonebook

phonebook: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

test_phonebook: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $(TESTSRC)

test: test_phonebook
	./test_phonebook

clean:
	rm -f phonebook test_phonebook

.PHONY: all test clean
