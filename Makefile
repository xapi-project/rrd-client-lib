# vim: ts=8 sw=8 noet:
#
#

CC 	= gcc
CFLAGS 	=
OBJ 	+= rrdlib.o

.PHONY: all
all: 	librdd.a

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f librdd.a

%.o: 	%.c
	$(CC) $(CFLAGS) -o $@ $<

librdd.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@


