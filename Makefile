#
# file:        Makefile
# description: makefile for CS 5600 Fall 2024 Lab 1
#

CFLAGS = -g

runsim: emulate.o runsim.o disasm.o
	$(CC) -g -o $@ $^

# disasm.c includes a main() function which is disabled unless you
# define this flag:
disasm: disasm.c
	$(CC) -g -o $@ -DSTANDALONE $^

test:	test.o emulate.o
	$(CC) -g -o $@ $^

clean:
	rm -f *.o runsim disasm test
