CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude
LDLIBS  = -lm

SRC     = src/calc.c src/command.c src/ui.c src/main.c
TESTSRC = src/calc.c src/command.c tests/test_calc.c

all: calculator

calculator: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

test_calc: $(TESTSRC)
	$(CC) $(CFLAGS) -o $@ $(TESTSRC) $(LDLIBS)

test: test_calc
	./test_calc

clean:
	rm -f calculator test_calc

.PHONY: all test clean
