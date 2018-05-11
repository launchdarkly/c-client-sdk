CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
CSRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c
COBJS=$(CSRCS:.c=.o)

LIBS=-lcurl -lpthread -lm

all: libldapi.so test

clean:
	rm -f *.o libldapi.so test

libldapi.so: ldapi.h ldinternal.h $(COBJS)
	$(CC) -o libldapi.so -fPIC -shared $(SRCS) $(LIBS)

test: ldapi.h test.o $(COBJS)
	$(CC) -o test test.o $(COBJS) $(LIBS)
