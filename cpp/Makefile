CC=gcc -fPIC -std=c99 -D_XOPEN_SOURCE=600 -I..
CXX=g++ -fPIC -D_XOPEN_SOURCE=600 -I..
UPCSRCS=ldclient.c ldutil.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c base64.c cJSON.c
CSRCS=$(addprefix ../,$(UPCSRCS))
CXXSRCS=ldcpp.cpp
COBJS=$(CSRCS:.c=.o)
CXXOBJS=$(CXXSRCS:.cpp=.o)
LIBS=-lcurl -lpthread -lm

all: libldapiplus.so testcpp

clean:
	rm -f *.o libldapiplus.so testcpp

libldapiplus.so: ../ldapi.h ../ldinternal.h $(COBJS) $(CXXOBJS)
	$(CXX) -o libldapiplus.so -fPIC -shared $(COBJS) $(CXXOBJS) $(LIBS)

testcpp: ../ldapi.h testcpp.o $(COBJS) $(CXXOBJS)
	$(CXX) -o testcpp testcpp.o $(COBJS) $(CXXOBJS) $(LIBS)
