# vim: ts=8 sw=8 noet:
#
#

CC	= gcc
CFLAGS	= -Wall
OBJ	+= librrd.o

.PHONY: all
all:	librrd.a

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f librrd.a
	rm -f librrd.o

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

librrd.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@

librrd.o: librrd.h
