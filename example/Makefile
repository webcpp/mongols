SERVERSRC=$(shell find . -type f -name '*.cpp')
SERVEROBJ=$(patsubst %.cpp,%.o,$(SERVERSRC))

CXX=g++
CXXFLAGS+=-std=c++11 -Wall -O3  `pkg-config --cflags mongols`
LDLIBS+=`pkg-config --libs mongols` -lpthread

all: server


server:
	@for i in $(SERVERSRC); do echo "${CXX} $${i}"; ${CXX} $${i} ${CXXFLAGS} ${LDLIBS}  -o `basename -s .cpp $${i}`;done


clean:
	@for i in $(SERVERSRC); do rm -f `basename -s .cpp $${i}`;done
	


