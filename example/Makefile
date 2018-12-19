SERVERSRC=$(shell find . -maxdepth 1 -type f -name '*.cpp' )
SERVEROBJ=$(patsubst %.cpp,%.o,$(SERVERSRC))

CXX=g++
CXXFLAGS+=-std=c++14 -Wall -O3  `pkg-config --cflags mongols`
LDLIBS+=`pkg-config --libs mongols`


js_c_module_path=`pwd`/html/js/package
chai_module_path=`pwd`/html/chai/package

all: server module

module:
	cd $(js_c_module_path)/adder && make clean && make 
	cd $(js_c_module_path)/concat && make clean && make
	cd $(chai_module_path)/test && make clean && make

server:
	@for i in $(SERVERSRC); do echo "${CXX} $${i}"; ${CXX} $${i} ${CXXFLAGS} ${LDLIBS}  -o `basename -s .cpp $${i}`;done


clean:
	@for i in $(SERVERSRC); do rm -f `basename -s .cpp $${i}`;done
	


