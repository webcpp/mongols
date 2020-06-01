SERVERSRC:=$(shell find . -maxdepth 1 -type f -name '*.cpp' )
SERVEROBJ:=$(patsubst %.cpp,%.o,$(SERVERSRC))

CXX=g++
CXXFLAGS+=-std=c++11 -Wall -O3  `pkg-config --cflags mongols openssl`
LDLIBS+=`pkg-config --libs mongols openssl`



lua_module_path=`pwd`/html/lua/package

all: server module

module:
	cd $(lua_module_path)/hello && make clean && make

server:
	@for i in $(SERVERSRC); do echo "${CXX} $${i}"; ${CXX} $${i} ${CXXFLAGS} ${LDLIBS}  -o `basename -s .cpp $${i}`;done


clean:
	@for i in $(SERVERSRC); do rm -f `basename -s .cpp $${i}`;done
	


