# vim: set ts=8 sw=8 noet:
#
#

CC	= gcc
CFLAGS	= -Wall
OBJ	+= librrd.o
OBJ 	+= parson/parson.o

.PHONY: all
all:	librrd.a rrdtest

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f librrd.a
	rm -f librrd.o
	rm -f parson/parson.o
	rm -f rrdtest.o
	rm -f rrdtest

parson:
	git submodule add https://github.com/kgabis/parson.git

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

librrd.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@

rrdtest: rrdtest.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^

librrd.o: librrd.h
parson/parson.o: parson/parson.h
