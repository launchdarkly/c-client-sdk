CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600 -I..
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
UPCSRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldfiler.c base64.c cJSON.c
CSRCS=$(addprefix ../,$(UPCSRCS))
COBJS=$(CSRCS:.c=.o)

LIBS=-lcurl -lpthread -lm

TESTS=test-private-attr.test test-filer.test test-hooks.test

all: $(TESTS) run-tests

clean:
	rm -f $(TESTS) *.o

%.test: %.o ../ldapi.h $(COBJS)
	$(CC) -o $@ $< $(COBJS) $(LIBS)

run-tests:
	./test-private-attr.test
	./test-filer.test
	./test-hooks.test
