CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
    SOEXT    = .dylib
    LDSHARED = -dynamiclib
    DL       =
else
    SOEXT    = .so
    LDSHARED = -shared
    DL       = -ldl          
endif

LIBDIR     = libs
PLUGIN_SRC = $(wildcard plugins/*.c)
PLUGIN_LIB = $(patsubst plugins/%.c,$(LIBDIR)/lib%$(SOEXT),$(PLUGIN_SRC))

APP_SRC = src/plugins.c src/ui.c src/main.c
APP_OBJ = $(APP_SRC:.c=.o)

all: calculator $(PLUGIN_LIB)

$(LIBDIR)/lib%$(SOEXT): plugins/%.c | $(LIBDIR)
	$(CC) $(CFLAGS) -fPIC $(LDSHARED) -o $@ $< -lm

$(LIBDIR):
	mkdir -p $(LIBDIR)

calculator: $(APP_OBJ)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJ) $(DL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_plugins: tests/test_plugins.o src/plugins.o
	$(CC) $(CFLAGS) -o $@ $^ $(DL)

test: test_plugins $(PLUGIN_LIB)
	./test_plugins

info: $(PLUGIN_LIB)
	@echo "--- библиотеки в $(LIBDIR)/ ---"
	@ls -1 $(LIBDIR)
	@echo "--- экспортируемые символы одной из них ---"
	@nm -g $(LIBDIR)/libadd$(SOEXT) | grep -E ' [TDS] ' | awk '{print $$3}' | sort

clean:
	rm -f calculator test_plugins $(APP_OBJ) tests/test_plugins.o
	rm -rf $(LIBDIR)

.PHONY: all test info clean
