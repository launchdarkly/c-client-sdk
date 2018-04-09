CC=gcc -std=c99 -D_XOPEN_SOURCE=500
SRCS=ldclient.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c

all: test

clean:
	rm test

test: test.c ldapi.h ldinternal.h $(SRCS)
	$(CC) -o test test.c $(SRCS) -lcurl -lpthread -lm

