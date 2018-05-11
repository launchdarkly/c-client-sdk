CC=gcc -std=c99 -D_XOPEN_SOURCE=600
CXX=g++ -D_XOPEN_SOURCE=600
SRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c
LIBS=-lcurl -lpthread -lm

all: test testcpp

clean:
	rm -f libldapi.so libldapiplus.so test testcpp

libldapi.so: ldapi.h ldinternal.h $(SRCS)
	$(CC) -o libldapi.so -fPIC -shared $(SRCS) $(LIBS)

libldapiplus.so: ldapi.h ldinternal.h $(SRCS) ldcpp.cpp
	$(CC) -o libldapiplus.so -fPIC -shared $(SRCS) -x c++ ldcpp.cpp $(LIBS)

test: test.c ldapi.h libldapi.so
	$(CC) -o test test.c $(SRCS) $(LIBS)

XCSRCS=$(addprefix -x c ,$(SRCS))
testcpp: testcpp.cpp ldapi.h libldapiplus.so
	$(CXX) -o testcpp testcpp.cpp ldcpp.cpp $(XCSRCS) $(LIBS)
