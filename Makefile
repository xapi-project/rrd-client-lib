# vim: set ts=8 sw=8 noet:
#
#

CC	= gcc
CFLAGS	= -g -Wall
OBJ	+= librrd.o
OBJ 	+= parson/parson.o
LIB     += -lz

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

.PHONY: test
test: 	rrdtest
	./rrdtest

.PHONY: valgrind
valgrind: rrdtest
	valgrind --leak-check=yes ./rrdtest

.PHONY: indent
indent: librrd.h librrd.c rrdtest.c
	indent -orig -nut $^

.PHONY: depend
depend: librrd.c rrdtest.c
	$(CC) -MM $^

.PHONY: parson
parson:
	# git submodule add https://github.com/kgabis/parson.git
	git submodule init
	git submodule update

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

librrd.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@

rrdtest: rrdtest.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

parson/parson.h: 	parson
parson/parson.c: 	parson

parson/parson.o: 	parson/parson.h
rrdtest.o: 		parson/parson.h librrd.h
librrd.o: 		parson/parson.h librrd.h
