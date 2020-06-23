.POSIX:
.SUFFIXES:

AR      = ar
CC      = cc
CFLAGS  = -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -fPIC -g -Og
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -Wno-unused
CFLAGS += -Isrc/
LDFLAGS =
LDLIBS  =

default: derzvim
all: libderzvim.a libderzvim.so derzvim derzvim_tests

libderzvim_sources =  \
  src/array.c         \
  src/term.c
libderzvim_objects = $(libderzvim_sources:.c=.o)

src/array.o: src/array.c src/array.h
src/term.o: src/term.c src/term.h src/array.h

libderzvim.a: $(libderzvim_objects)
	@echo "STATIC  $@"
	@$(AR) rcs $@ $(libderzvim_objects)

libderzvim.so: $(libderzvim_objects)
	@echo "SHARED  $@"
	@$(CC) $(LDFLAGS) -shared -o $@ $(libderzvim_objects) $(LDLIBS)

derzvim: src/main.c libderzvim.a
	@echo "EXE     $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/main.c libderzvim.a $(LDLIBS)

derzvim_tests_sources =  \
  src/array_test.c

derzvim_tests: $(derzvim_tests_sources) src/main_test.c libderzvim.a
	@echo "EXE     $@"
	@$(CC) $(CFLAGS) -o $@ src/main_test.c libderzvim.a

.PHONY: run
run: derzvim
	./derzvim

.PHONY: check
check: derzvim_tests
	./derzvim_tests

.PHONY: clean
clean:
	rm -fr derzvim derzvim_tests *.a *.so src/*.o

.SUFFIXES: .c .o
.c.o:
	@echo "CC      $@"
	@$(CC) $(CFLAGS) -c -o $@ $<
