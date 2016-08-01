# vim: set ts=8 sw=8 noet:
#
#

PATH    := $(PATH):/tmp/cov-analysis-linux64-8.5.0.1/bin

CC	= gcc
CFLAGS	= -std=gnu99 -g -Wall
OBJ	+= librrd.o
OBJ 	+= parson/parson.o
LIB     += -lz

.PHONY: all
all:	librrd.a rrdtest rrdclient

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f librrd.a
	rm -f librrd.o
	rm -f parson/parson.o
	rm -f rrdtest.o rrdtest
	rm -f rrdclient.o rrdclient

.PHONY: test
test: 	rrdtest rrdclient
	./rrdtest
	seq 1 10 | ./rrdclient rrdclient.rrd
	seq 1 10 \
		| while read i; do echo $$i; sleep 1; done \
		| ./rrdclient rrdclient.rrd

.PHONY: valgrind
valgrind: rrdtest
	valgrind --leak-check=yes ./rrdtest
	seq 1 10 | valgrind --leak-check=yes ./rrdclient rrdclient.rrd

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

rrdclient: rrdclient.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

# coverity analysis
cov: 	parson
	cov-configure --gcc --config config.xml
	cov-build --dir cov-int --config config.xml $(MAKE)
	cov-analyze --dir cov-int --config config.xml
	cov-format-errors --dir cov-int --emacs-style > coverity.out

parson/parson.h: 	parson
parson/parson.c: 	parson

parson/parson.o: 	parson/parson.h
rrdtest.o: 		parson/parson.h librrd.h
librrd.o: 		parson/parson.h librrd.h
