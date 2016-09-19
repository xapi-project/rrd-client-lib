# vim: set ts=8 sw=8 noet:
#
# To run the integration tests you need the ocaml-rrd-transport-devel
# package:
#
# yum install -y ocaml-rrd-transport-devel
#
# We don't have an install target because installation is handled
# from the spec file.


NAME    = rrd-client-lib
VERSION = 1.1.0
OS 	:= $(shell uname)

CC	= gcc
CFLAGS	= -std=gnu99 -g -fpic -Wall
OBJ	+= librrd.o
OBJ 	+= parson/parson.o
LIB     += -lz

ifeq ($(OS),Darwin)
LDFLAGS = -shared -Wl
else
LDFLAGS = -shared -Wl,--version-script=version.script
endif

.PHONY: all
all:	librrd.a librrd.so rrdtest rrdclient

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f librrd.a
	rm -f librrd.o
	rm -f librrd.so
	rm -f parson/parson.o
	rm -f rrdtest.o rrdtest
	rm -f rrdclient.o rrdclient
	rm -rf config.xml cov-int html coverity.out

.PHONY: test
test: 	rrdtest rrdclient
	./rrdtest
	seq 1 10 | ./rrdclient rrdclient.rrd
	test ! -f rrdclient.rrd

.PHONY: test-integration
test-integration: rrdclient
	seq 1 10 | while read i; do echo $$i ; sleep 4; done \
		| ./rrdclient rrdclient.rrd &
	rrdreader file rrdclient.rrd v2 \
		|| echo "a final exception Rrd_protocol.No_update is OK"
	test ! -f rrdclient.rrd

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

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

librrd.a: $(OBJ)
	ar rc $@ $(OBJ)
	ranlib $@

librrd.so: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIB)

rrdtest: rrdtest.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

rrdclient: rrdclient.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

.PHONY: tar
tar:
	git archive --format=tar --prefix=$(NAME)-$(VERSION)/  HEAD\
		| gzip > $(NAME)-$(VERSION).tar.gz

# coverity analysis

COV_OPTS += --cpp
COV_OPTS += --aggressiveness-level high
COV_OPTS += --all
COV_OPTS += --rule
COV_OPTS += --disable-parse-warnings
COV_OPTS += --enable-fnptr

COV_DIR = cov-int

cov: 	parson
	cov-configure --gcc --config config.xml
	cov-build --dir $(COV_DIR)  --config config.xml $(MAKE)
	cov-analyze $(COV_OPTS) --dir $(COV_DIR) --config config.xml
	cov-format-errors --dir $(COV_DIR)  --emacs-style > coverity.out
	cov-format-errors --dir $(COV_DIR) --html-output html

# C dependencies

parson/parson.h: 	parson
parson/parson.c: 	parson

parson/parson.o: 	parson/parson.h
rrdtest.o: 		parson/parson.h librrd.h
librrd.o: 		parson/parson.h librrd.h

