CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600 -I..
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
UPCSRCS=ldclient.c ldutil.c lduser.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldstore.c base64.c cJSON.c
CSRCS=$(addprefix ../,$(UPCSRCS))
COBJS=$(CSRCS:.c=.o)

LIBS=-lcurl -lpthread -lm

TESTS=test-store.test test-hooks.test test-userencoding.test

all: run-tests

clean:
	rm -f $(TESTS) *.o

%.test: %.o ../ldapi.h $(COBJS)
	$(CC) -o $@ $< $(COBJS) $(LIBS)

run-tests: $(TESTS)
	for t in $(TESTS) ; do ./$$t || echo FAIL: $$t ; done
