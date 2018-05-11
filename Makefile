CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600
CXX=g++ -fPIC -D_XOPEN_SOURCE=600
CSRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c
CXXSRCS=ldcpp.cpp
COBJS=$(CSRCS:.c=.o)
CXXOBJS=$(CXXSRCS:.cpp=.o)
LIBS=-lcurl -lpthread -lm

all: libldapi.so libldapiplus.so test testcpp

clean:
	rm -f *.o libldapi.so libldapiplus.so test testcpp

libldapi.so: ldapi.h ldinternal.h $(COBJS)
	$(CC) -o libldapi.so -fPIC -shared $(SRCS) $(LIBS)

libldapiplus.so: ldapi.h ldinternal.h $(COBJS) $(CXXOBJS)
	$(CXX) -o libldapiplus.so -fPIC -shared $(COBJS) $(CXXOBJS) $(LIBS)

test: ldapi.h test.o $(COBJS)
	$(CC) -o test test.o $(COBJS) $(LIBS)

testcpp: ldapi.h testcpp.o $(COBJS) $(CXXOBJS)
	$(CXX) -o testcpp testcpp.o $(COBJS) $(CXXOBJS) $(LIBS)
