CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600 -I..
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
UPCSRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldfiler.c base64.c cJSON.c
CSRCS=$(addprefix ../,$(UPCSRCS))
COBJS=$(CSRCS:.c=.o)

LIBS=-lcurl -lpthread -lm

TESTS=test-private-attr test-filer

all: $(TESTS) run-tests

clean:
	rm -f $(TESTS)

test-private-attr: ../ldapi.h test-private-attr.o $(COBJS)
	$(CC) -o test-private-attr test-private-attr.o $(COBJS) $(LIBS)

test-filer: ../ldapi.h test-filer.o $(COBJS)
	$(CC) -o test-filer test-filer.o $(COBJS) $(LIBS)

run-tests:
	./test-private-attr
	./test-filer
