#
# Makefile for vista library
#

# Use the following line of flags if your application calls mprotect()
#CFLAGS = -O2 -DVISTAGRAMS -DAPP_MPROTECT

CFLAGS = -O2 -DVISTAGRAMS
LIBDIR = ../lib
BINDIR = ../bin


OBJ_FILES = heap.o hash.o vistagrams.o vista.o lock.o

default: install

vistatest: vistatest.c lib
	cc ${CFLAGS} -o vistatest vistatest.c -L. -lvista

lib: ${OBJ_FILES}
	@echo
	ar rv libvista.a ${OBJ_FILES}
	ranlib libvista.a
	@echo

libvista.a: lib

install: lib vistatest
	cp libvista.a ${LIBDIR}
	cp vistatest ${BINDIR}

clean:
	rm -f vistatest *.a ${OBJ_FILES}

