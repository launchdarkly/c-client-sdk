AR=ar rcs
SHAREDFLAGS=-fPIC -D_XOPEN_SOURCE=600 -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden
CFLAGS=$(SHAREDFLAGS) -std=c99
CXXFLAGS=$(SHAREDFLAGS)
CC=gcc
CXX=g++
CSRCS=ldclient.c ldutil.c lduser.c ldthreads.c ldlog.c ldnet.c ldevents.c ldhash.c ldstore.c base64.c cJSON.c
COBJS=$(CSRCS:.c=.o)
CXXSRCS=ldcpp.cpp
CXXOBJS=$(CXXSRCS:.cpp=.o)

LIBS=-lcurl -lpthread -lm

UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
	LIBS+= -framework CoreFoundation -framework IOKit
endif

ifeq ($(ALTBUILD),mingw)
	CURLPATH=curl-7.62.0-win64-mingw
	LIBS+= -lwsock32 -L $(CURLPATH)/lib
	AR=x86_64-w64-mingw32-gcc-ar rcs
	SHAREDFLAGS+= -D_WINDOWS -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -I $(CURLPATH)/include
	CC=x86_64-w64-mingw32-gcc
	CXX=x86_64-w64-mingw32-g++
endif

all: libldapi.so libldapi.a libldapiplus.so test

clean:
	rm -f *.o *.so *.a test

libldapi.a: ldapi.h ldinternal.h $(COBJS)
	$(AR) libldapi.a $(COBJS)

libldapi.so: ldapi.h ldinternal.h $(COBJS)
	$(CC) -o libldapi.so -fPIC -shared $(COBJS) $(LIBS)

libldapiplus.so: ldapi.h ldinternal.h $(COBJS) $(CXXOBJS)
	$(CXX) -o libldapiplus.so -fPIC -shared $(COBJS) $(CXXOBJS) $(LIBS)

test: ldapi.h test.o $(COBJS)
	$(CC) -o test test.o $(COBJS) $(LIBS)
