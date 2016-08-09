# vim: set ts=8 sw=8 noet:
#
# To run the integration tests you need the ocaml-rrd-transport-devel
# package:
#
# yum install -y ocaml-rrd-transport-devel
#

CC	= gcc
CFLAGS	= -std=gnu99 -g -Wall
OBJ	+= librrd.o
OBJ 	+= parson/parson.o
LIB     += -lz

OCB 	= ocamlbuild -use-ocamlfind

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
	rm -rf config.xml cov-int html coverity.out
	cd ocaml;  $(OCB) -clean

.PHONY: test
test: 	rrdtest rrdclient
	./rrdtest
	seq 1 10 | ./rrdclient rrdclient.rrd

.PHONY: test-integration
test-integration: 
	seq 1 10 | while read i; do \
		echo $$i | ./rrdclient rrdclient.rrd ;\
		rrdreader file --once rrdclient.rrd v2 ;\
	done
	seq 1 20 | while read i; do \
		echo $$i | ./rrdclient rrdclient.rrd ;\
		sleep 4 ;\
	done &  rrdreader file rrdclient.rrd v2 \
	|| echo "a final exception Rrd_protocol.No_update is OK"

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

rrdtest: rrdtest.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

rrdclient: rrdclient.o librrd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

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

