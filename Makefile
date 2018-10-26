AR=ar rcs
CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
CSRCS=ldclient.c ldutil.c lduser.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldstore.c base64.c cJSON.c
COBJS=$(CSRCS:.c=.o)
CXXSRCS=ldcpp.cpp
CXXOBJS=$(CXXSRCS:.cpp=.o)

LIBS=-lcurl -lpthread -lm

all: libldapi.so libldapiplus.so libldapi.a  test

clean:
	rm -f *.o libldapi.so libldapi.a test

libldapi.a: ldapi.h ldinternal.h $(COBJS)
	$(AR) libldapi.a $(COBJS)

libldapi.so: ldapi.h ldinternal.h $(COBJS)
	$(CC) -o libldapi.so -fPIC -shared $(SRCS) $(LIBS)

libldapiplus.so: ldapi.h ldinternal.h $(COBJS) $(CXXOBJS)
	$(CXX) -o libldapiplus.so -fPIC -shared $(COBJS) $(CXXOBJS) $(LIBS)

test: ldapi.h test.o $(COBJS)
	$(CC) -o test test.o $(COBJS) $(LIBS)
