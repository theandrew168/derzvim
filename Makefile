.POSIX:
.SUFFIXES:

AR      = ar
CC      = cc
CFLAGS  = -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -fPIC -g -Og
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -Wno-unused
CFLAGS += -Isrc/ -I/usr/include/lua5.1
LDFLAGS =
LDLIBS  = -llua5.1
PREFIX  = /usr/local

default: derzvim
all: libderzvim.a libderzvim.so derzvim derzvim_tests

libderzvim_sources =  \
  src/editor.c        \
  src/line.c          \
  src/term.c
libderzvim_objects = $(libderzvim_sources:.c=.o)

src/editor.o: src/editor.c src/editor.h src/line.h src/term.h
src/line.o: src/line.c src/line.h
src/term.o: src/term.c src/term.h

libderzvim.a: $(libderzvim_objects)
	@echo "STATIC  $@"
	@$(AR) rcs $@ $(libderzvim_objects)

libderzvim.so: $(libderzvim_objects)
	@echo "SHARED  $@"
	@$(CC) $(LDFLAGS) -shared -o $@ $(libderzvim_objects) $(LDLIBS)

derzvim: src/main.c libderzvim.a
	@echo "EXE     $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/main.c libderzvim.a $(LDLIBS)

derzvim_tests_sources =

derzvim_tests: $(derzvim_tests_sources) src/main_test.c libderzvim.a
	@echo "EXE     $@"
	@$(CC) $(CFLAGS) -o $@ src/main_test.c libderzvim.a

.PHONY: check
check: derzvim_tests
	./derzvim_tests

.PHONY: install
install: derzvim
	mkdir -p $(PREFIX)/bin
	install -m 755 derzvim $(PREFIX)/bin

.PHONY: clean
clean:
	rm -fr derzvim derzvim_tests *.a *.so src/*.o

.SUFFIXES: .c .o
.c.o:
	@echo "CC      $@"
	@$(CC) $(CFLAGS) -c -o $@ $<
