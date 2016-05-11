
VPATH = ../../include

#  PIN paths
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
BINS = wraptrace.so

wraptrace.so: wraptrace.cpp debug.h simcache.h pin_cache_wrap.H simnvm.h nvmain.a simdram.h dramsim.a
	g++ $(PIN_C_OPTIONS) -o wraptrace.o -I../nvmain -I../../include -I../dramsim2 wraptrace.cpp
	g++ $(PIN_L_OPTIONS) -o wraptrace.so wraptrace.o pmem.o scmio.o memtool.o nvmain.a dramsim.a -lpin -lxed -ldwarf -lelf -ldl -lrt

allcache.so: allcache.cpp pin_cache.H
	g++ $(PIN_C_OPTIONS) -o allcache.o allcache.cpp
	g++ $(PIN_L_OPTIONS) -o allcache.so allcache.o -lpin -lxed -ldwarf -lelf -ldl

allcache_wrap.so: allcache_wrap.cpp simcache.h pin_cache_wrap.H debug.h
	g++ $(PIN_C_OPTIONS) -o allcache_wrap.o allcache_wrap.cpp
	g++ $(PIN_L_OPTIONS) -o allcache_wrap.so allcache_wrap.o -lpin -lxed -ldwarf -lelf -ldl
	
nvmainpin.so: nvmainpin.cpp nvmain.a simnvm.h debug.h
	g++ $(PIN_C_OPTIONS) -o nvmainpin.o -I../nvmain nvmainpin.cpp
	g++ $(PIN_L_OPTIONS) -o nvmainpin.so nvmainpin.o nvmain.a -lpin -lxed -ldwarf -lelf -ldl

dramsimpin.so: dramsimpin.cpp dramsim.a simdram.h debug.h memtool.o 
	g++ $(PIN_C_OPTIONS) -o dramsimpin.o -I../dramsim2 dramsimpin.cpp
	g++ $(PIN_L_OPTIONS) -o dramsimpin.so dramsimpin.o memtool.o dramsim.a -lpin -lxed -ldwarf -lelf -ldl
	
clean:
	rm -f $(BINS)
