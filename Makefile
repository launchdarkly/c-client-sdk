CC=gcc -std=c99 -D_XOPEN_SOURCE=600
CXX=g++ -D_XOPEN_SOURCE=600
SRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c

all: test testcpp

clean:
	rm -f libldapi.so libldapiplus.so test testcpp

libldapi.so: ldapi.h ldinternal.h $(SRCS)
	$(CC) -o libldapi.so -fPIC -shared $(SRCS)

libldapiplus.so: ldapi.h ldinternal.h $(SRCS) ldcpp.cpp
	$(CC) -o libldapiplus.so -fPIC -shared $(SRCS) -x c++ ldcpp.cpp

test: test.c ldapi.h libldapi.so
	$(CC) -o test test.c -Wl,-rpath=. libldapi.so -lcurl -lpthread -lm

testcpp: testcpp.cpp ldapi.h libldapiplus.so
	$(CXX) -o testcpp testcpp.cpp -Wl,-rpath=. libldapiplus.so -lcurl -lpthread -lm
