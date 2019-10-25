CC=gcc -g3 -fPIC -std=c99 -D_XOPEN_SOURCE=600 -I.. -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
UPCSRCS=ldclient.c ldutil.c lduser.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldstore.c base64.c cJSON.c
CSRCS=$(addprefix ../,$(UPCSRCS))
COBJS=$(CSRCS:.c=.o)

LIBS=-lcurl -lpthread -lm

UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
	LIBS+= -framework CoreFoundation -framework IOKit
endif

TESTS=test-store.test test-hooks.test test-userencoding.test test-deviceid.test test-multienv.test test-events.test test-variations.test

all: run-tests

clean:
	rm -f $(TESTS) *.o

%.test: %.o ../ldapi.h $(COBJS)
	$(CC) -o $@ $< $(COBJS) $(LIBS)

run-tests: $(TESTS)
	for t in $(TESTS) ; do echo "Running: $$t" && ./$$t || echo FAIL: $$t ; done
