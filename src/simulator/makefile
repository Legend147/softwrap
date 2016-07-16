
VPATH   = ../../include
DRAMSIM = ../../../dramsim2
NVMAIN  = ../../../nvmain

DRAMSIM_LIB = $(DRAMSIM)/dramsim.a
NVMAIN_LIB  = $(NVMAIN)/nvmain.a

#  PIN paths
ifndef $(PIN_PATH)
PIN_PATH = ../../../pin
endif
PIN = $(PIN_PATH)/pin
PIN_INC = $(PIN_PATH)/source/include/pin
PIN_DEBUG = -g
PIN_C_OPTIONS = -c -Wall -Werror -Wno-unknown-pragmas -fomit-frame-pointer -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -fno-strict-aliasing \
		-I$(PIN_PATH)/source/tools/InstLib -I$(PIN_PATH)/extras/xed2-intel64/include -I$(PIN_INC)/gen \
	 	-I$(PIN_PATH)/extras/components/include -I$(PIN_PATH)/source/include -I$(PIN_INC) \
	 	-fno-stack-protector -fPIC -DTARGET_LINUX $(PIN_DEBUG)  -fomit-frame-pointer \
	 	-DTARGET_IA32E -DHOST_IA32E 
PIN_L_OPTIONS = -Wl,-rpath=$(PIN_PATH)/intel64/runtime/cpplibs -Wl,--hash-style=sysv -shared -Wl,-Bsymbolic \
		-Wl,--version-script=$(PIN_INC)/pintool.ver -L$(PIN_PATH)/intel64/runtime/cpplibs \
		-L$(PIN_PATH)/extras/xed2-intel64/lib -L$(PIN_PATH)/intel64/lib -L$(PIN_PATH)/intel64/lib-ext  \
		-L$(PIN_PATH)/intel64/runtime/cpplibs -L$(PIN_PATH)/extras/xed2-intel64/lib \
		-L$(PIN_PATH)/intel64/lib -L$(PIN_PATH)/intel64/lib-ext $(PIN_DEBUG)

LIBS = wraptrace.so allcache.so allcache_wrap.so nvmainpin.so dramsimpin.so

all: $(LIBS)

clean:
	rm -f $(LIBS)
	rm -f *.o

BINS = wraptrace.so

wraptrace.so: wraptrace.cpp debug.h simcache.h pin_cache_wrap.H simnvm.h $(NVMAIN_LIB) simdram.h $(DRAMSIM_LIB)
	g++ $(PIN_C_OPTIONS) -o wraptrace.o -I$(NVMAIN) -I../main -I../../include -I$(DRAMSIM) wraptrace.cpp
	g++ $(PIN_L_OPTIONS) -o wraptrace.so $(NVMAIN_LIB) $(DRAMSIM_LIB) ../../lib/libwrap.so -lpin -lxed -ldwarf -lelf -ldl -lrt

allcache.so: allcache.cpp pin_cache.H
	g++ $(PIN_C_OPTIONS) -o allcache.o allcache.cpp
	g++ $(PIN_L_OPTIONS) -o allcache.so allcache.o -lpin -lxed -ldwarf -lelf -ldl

allcache_wrap.so: allcache_wrap.cpp simcache.h pin_cache_wrap.H debug.h
	g++ $(PIN_C_OPTIONS) -o allcache_wrap.o -I../main -I../../include allcache_wrap.cpp
	g++ $(PIN_L_OPTIONS) -o allcache_wrap.so allcache_wrap.o -lpin -lxed -ldwarf -lelf -ldl
	
nvmainpin.so: nvmainpin.cpp $(NVMAIN_LIB) simnvm.h debug.h
	g++ $(PIN_C_OPTIONS) -o nvmainpin.o -I$(NVMAIN) -I../main -I../../include nvmainpin.cpp
	g++ $(PIN_L_OPTIONS) -o nvmainpin.so nvmainpin.o $(NVMAIN_LIB) -lpin -lxed -ldwarf -lelf -ldl

dramsimpin.so: dramsimpin.cpp $(DRAMSIM_LIB) simdram.h debug.h
	g++ $(PIN_C_OPTIONS) -o dramsimpin.o -I$(DRAMSIM) -I../main -I../../include dramsimpin.cpp
	g++ $(PIN_L_OPTIONS) -o dramsimpin.so dramsimpin.o $(DRAMSIM_LIB) -lpin -lxed -ldwarf -lelf -ldl
	
clean:
	rm -f $(BINS)
