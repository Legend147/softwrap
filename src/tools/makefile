
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
BINS = wraptool.so

wraptool.so: wraptool.cpp debug.h 
	g++ $(PIN_C_OPTIONS) -o wraptool.o -I../../include wraptool.cpp
	g++ $(PIN_L_OPTIONS) -o wraptool.so wraptool.o -lpin -lxed -ldwarf -lelf -ldl -lrt

clean:
	rm -f $(BINS)
