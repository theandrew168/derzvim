.POSIX:
.SUFFIXES:

AR      = ar
CC      = cc
CFLAGS  = -std=c99 -D_POSIX_C_SOURCE=200112L
CFLAGS += -fPIC -g -Og
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -Wno-unused
CFLAGS += -Isrc/
LDFLAGS =
LDLIBS  =

default: derzvim
all: libderzvim.a libderzvim.so derzvim

libderzvim_sources =  \
  src/derzvim.c
libderzvim_objects = $(libderzvim_sources:.c=.o)

src/derzvim.o: src/derzvim.c src/derzvim.h

libderzvim.a: $(libderzvim_objects)
	@echo "STATIC  $@"
	@$(AR) rcs $@ $(libderzvim_objects)

libderzvim.so: $(libderzvim_objects)
	@echo "SHARED  $@"
	@$(CC) $(LDFLAGS) -shared -o $@ $(libderzvim_objects) $(LDLIBS)

derzvim: src/main.c libderzvim.a
	@echo "EXE     $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/main.c libderzvim.a $(LDLIBS)

.PHONY: clean
clean:
	rm -fr derzvim derzvim_tests *.a *.so src/*.o

.SUFFIXES: .c .o
.c.o:
	@echo "CC      $@"
	@$(CC) $(CFLAGS) -c -o $@ $<
