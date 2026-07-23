CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Ilib/include -Iapp/include

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
    LIB      = libphonebook.dylib
    LDSHARED = -dynamiclib -install_name @rpath/$(LIB)
    RPATH    = -Wl,-rpath,@loader_path
else
    LIB      = libphonebook.so
    LDSHARED = -shared
    RPATH    = -Wl,-rpath,'$$ORIGIN'
endif

PICFLAGS = -fPIC

LIB_SRC  = lib/src/contact.c lib/src/utf8.c lib/src/phonebook.c
LIB_OBJ  = $(LIB_SRC:.c=.o)

APP_SRC  = app/src/storage.c app/src/ui.c app/src/main.c
APP_OBJ  = $(APP_SRC:.c=.o)

TEST_SRC = tests/test_phonebook.c app/src/storage.c
TEST_OBJ = $(TEST_SRC:.c=.o)

all: phonebook

$(LIB): $(LIB_OBJ)
	$(CC) $(LDSHARED) -o $@ $^

phonebook: $(APP_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJ) -L. -lphonebook $(RPATH)

test_phonebook: $(TEST_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) -L. -lphonebook $(RPATH)

lib/src/%.o: lib/src/%.c
	$(CC) $(CFLAGS) $(PICFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: test_phonebook
	./test_phonebook

info: phonebook
	@echo "--- от каких библиотек зависит ./phonebook ---"
ifeq ($(UNAME),Darwin)
	@otool -L phonebook | tail -n +2
else
	@ldd phonebook
endif
	@echo "--- экспортируемые функции $(LIB) ---"
	@nm -g $(LIB) | grep ' T ' | awk '{print $$3}' | sort

clean:
	rm -f $(LIB) $(LIB_OBJ) $(APP_OBJ) $(TEST_OBJ) phonebook test_phonebook

.PHONY: all test info clean
