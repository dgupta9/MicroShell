#	Makefile for Program Threading library - CSC 501 Assignment 2 
#
#	By Durgesh Kumar Gupta
#	dgupta9@ncsu.edu
#	make file from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
#

CC_C= gcc
CFLAGS = -Wall -g -Werror

TARGETS= ush
DEPS = parse.h
OBJ = main.o parse.o

$(TARGETS): $(OBJ) $(DEPS)
	$(CC_C) -o $@ $^ $(CFLAGS)
	rm -f *.o

%.o: %.c $(DEPS)
	$(CC_C) -c  $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -f $(TARGETS) $(OBJ)
