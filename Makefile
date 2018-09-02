NAME=mongols
PROJECT=lib$(NAME).so
CPPSRC=$(shell find . -type f -name *.cpp | sed -e 's/.*indexer\.cpp$$//')
CPPOBJ=$(patsubst %.cpp,%.o,$(CPPSRC))
CCSRC=$(shell find . -type f -name *.cc)
CCOBJ=$(patsubst %.cc,%.o,$(CCSRC))
CXXSRC=$(shell find . -type f -name *.cxx)
CXXOBJ=$(patsubst %.cxx,%.o,$(CXXSRC))

CSRC=$(shell find . -type f -name *.c | sed -e 's/.*indexer.c$$//')
COBJ=$(patsubst %.c,%.o,$(CSRC))

OBJ=$(COBJ) $(CXXOBJ) $(CCOBJ) $(CPPOBJ)

CC=gcc
CXX=g++

CFLAGS+=-O3 -std=c11 -Wall -fPIC
CFLAGS+=-Iinc/mongols -Iinc/mongols/lib
CFLAGS+=-Iinc/mongols/lib/lua -Wextra -DLUA_COMPAT_5_2 -DLUA_USE_POSIX
CFLAGS+=-Iinc/mongols/lib/sqlite  -DSQLITE_THREADSAFE=1 \
	-DSQLITE_ENABLE_FTS4  \
	-DSQLITE_ENABLE_FTS5 \
	-DSQLITE_ENABLE_JSON1  \
	-DSQLITE_ENABLE_RTREE \
	-DSQLITE_ENABLE_EXPLAIN_COMMENTS  \
	-DHAVE_USLEEP \
	-DHAVE_READLINE

CFLAGS+=`pkg-config --cflags openssl`



CXXFLAGS+=-O3 -std=c++11 -Wall -fPIC 
CXXFLAGS+=-Iinc/mongols -Iinc/mongols/lib 
CXXFLAGS+=-Isrc/MPFDParser
CXXFLAGS+=-Iinc/mongols/lib/leveldb -Isrc/leveldb -DLEVELDB_PLATFORM_POSIX
CXXFLAGS+= -DKAGUYA_USE_CPP11
CXXFLAGS+=-Isrc -Isrc/re2 
CXXFLAGS+=-Iinc/mongols/lib/sqlite

CXXFLAGS+=`pkg-config --cflags openssl`


LDLIBS+=`pkg-config --libs openssl`
LDLIBS+=-lpcre -lz -lpthread -ldl -lrt -lm -lstdc++
LDFLAGS+=-shared


ifndef INSTALL_DIR
INSTALL_DIR=/usr/local
endif


all:$(PROJECT)

$(PROJECT):$(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) 

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS)  -c $< -o $@

.cc.o:
	$(CXX) $(CXXFLAGS)  -c $< -o $@
	
.cxx.o:
	$(CXX) $(CXXFLAGS)  -c $< -o $@

clean:
	@for i in $(OBJ);do echo "rm -f" $${i} && rm -f $${i} ;done
	rm -f $(PROJECT)

install:
	test -d $(INSTALL_DIR)/ || mkdir -p $(INSTALL_DIR)/
	install $(PROJECT) $(INSTALL_DIR)/lib
	cp -R inc/mongols $(INSTALL_DIR)/include
	mkdir -pv $(INSTALL_DIR)/lib/pkgconfig
	install mongols.pc $(INSTALL_DIR)/lib/pkgconfig

