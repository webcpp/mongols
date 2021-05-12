NAME=mongols
PROJECT=lib$(NAME).so
CPPSRC:=$(shell find src -type f -name *.cpp)
CPPOBJ:=$(patsubst %.cpp,%.o,$(CPPSRC))
CCSRC:=$(shell find src -type f -name *.cc)
CCOBJ:=$(patsubst %.cc,%.o,$(CCSRC))
CXXSRC:=$(shell find src -type f -name *.cxx)
CXXOBJ:=$(patsubst %.cxx,%.o,$(CXXSRC))

CSRC:=$(shell find src -type f -name *.c)
COBJ:=$(patsubst %.c,%.o,$(CSRC))

OBJ:=$(COBJ) $(CXXOBJ) $(CCOBJ) $(CPPOBJ)

CC=gcc
CXX=g++

CFLAGS+=-O3 -g -std=c11 -fPIC -DNDEBUG
CXXFLAGS+=-O3 -g -std=c++11 -fPIC -DNDEBUG


QUICKJS_VERSION:=$(shell cat src/qjs/VERSION)

BOTHFLAGS=-Wall -Wextra -Werror \
		  -Wno-sign-compare     \
		  -Wno-missing-field-initializers \
		  -Wno-unused-but-set-variable \
		  -Wno-unused-label \
    	  -Wno-unused-function \
    	  -Wno-unused-parameter \
    	  -Wno-unused-variable \
		  -Wno-array-bounds \
		  -Wno-maybe-uninitialized \
		  -Wno-implicit-fallthrough \
		  -Wno-stringop-truncation \
		  -Wno-cast-function-type \
		  -Wno-deprecated-declarations \
		  -Wno-unused-result \
		  -Wno-write-strings



BOTHFLAGS+=`pkg-config --cflags openssl`
BOTHFLAGS+=-Iinc/mongols -Iinc/mongols/lib
BOTHFLAGS+=-Iinc/mongols/lib/lua -DLUA_COMPAT_5_3 -DLUA_USE_LINUX -D_GNU_SOURCE -DKAGUYA_USE_CPP11
BOTHFLAGS+=-DMULTIPLE_THREADS
BOTHFLAGS+=-Iinc/mongols/lib/sqlite  -DSQLITE_THREADSAFE=1 \
	-DSQLITE_ENABLE_FTS4  \
	-DSQLITE_ENABLE_FTS5 \
	-DSQLITE_ENABLE_JSON1  \
	-DSQLITE_ENABLE_RTREE \
	-DSQLITE_ENABLE_EXPLAIN_COMMENTS  \
	-DHAVE_USLEEP \
	-DHAVE_READLINE
BOTHFLAGS+=-Iinc/mongols/lib/z
BOTHFLAGS+=-Iinc/mongols/lib/hash
BOTHFLAGS+=-Iinc/mongols/lib/qjs -D_GNU_SOURCE -DCONFIG_VERSION=\"$(QUICKJS_VERSION)\" -DCONFIG_BIGNUM
BOTHFLAGS+=-Iinc/mongols/lib/hiredis
BOTHFLAGS+=-Iinc/mongols/lib/leveldb -Isrc/leveldb -DLEVELDB_PLATFORM_POSIX -DLEVELDB_HAS_PORT_CONFIG_H
BOTHFLAGS+=-Isrc/MPFDParser
BOTHFLAGS+=-Isrc -Isrc/re2

CFLAGS+=$(BOTHFLAGS)
CXXFLAGS+=$(BOTHFLAGS)


LDLIBS+=-lpthread -ldl -lrt -lm -lstdc++
LDLIBS+=`pkg-config --libs openssl`
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

