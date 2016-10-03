
VPATH = ../../include

#  PIN paths
PIN = $(PIN_PATH)/pin
PIN_INC = $(PIN_PATH)/source/include/pin
PIN_DEBUG = -g
PIN_C_OPTIONS = -c -Wall -Werror -Wno-unknown-pragmas -fomit-frame-pointer -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -fno-strict-aliasing \
		-I$(PIN_PATH)/source/tools/InstLib -I$(PIN_PATH)/extras/xed2-intel64/include -I$(PIN_PATH)/extras/xed2-intel64/include \
		-I$(PIN_INC)/gen \
	 	-I$(PIN_PATH)/extras/components/include -I$(PIN_PATH)/source/include -I$(PIN_INC) \
	 	-fno-stack-protector -fPIC -DTARGET_LINUX $(PIN_DEBUG)  -fomit-frame-pointer \
	 	-DTARGET_IA32E -DHOST_IA32E 
PIN_L_OPTIONS = -Wl,-rpath=$(PIN_PATH)/intel64/runtime/cpplibs -Wl,--hash-style=sysv -shared -Wl,-Bsymbolic \
		-Wl,--version-script=$(PIN_INC)/pintool.ver -L$(PIN_PATH)/intel64/runtime/cpplibs \
		-L$(PIN_PATH)/extras/xed2-intel64/lib -L$(PIN_PATH)/extras/xed-intel64/lib -L$(PIN_PATH)/intel64/lib -L$(PIN_PATH)/intel64/lib-ext  \
		$(PIN_DEBUG)

#PIN_C_OPTIONS = -DTARGET_IA32E -DHOST_IA32E -Wall -Werror -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fPIC -DTARGET_LINUX -I$(PIN_PATH)/source/include/pin -I$(PIN_PATH)/source/include/pin/gen -isystem \
	$(PIN_PATH)/extras/stlport/include -isystem $(PIN_PATH)/extras/libstdc++/include -isystem $(PIN_PATH)/extras/crt/include -isystem $(PIN_PATH)/extras/crt/include/arch-x86_64 -isystem $(PIN_PATH)/extras/crt/include/kernel/uapi -isystem $(PIN_PATH)/extras/crt/include/kernel/uapi/asm-x86 -I$(PIN_PATH)/extras/components/include -I$(PIN_PATH)/extras/xed-intel64/include -I$(PIN_PATH)/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing   -c
 
#PIN_L_OPTIONS = -shared -Wl,--hash-style=sysv $(PIN_PATH)/intel64/runtime/pincrt/crtbeginS.o -Wl,-Bsymbolic -Wl,--version-script=$(PIN_PATH)/source/include/pin/pintool.ver -fabi-version=2    -L$(PIN_PATH)/intel64/runtime/pincrt -L$(PIN_PATH)/intel64/lib -L$(PIN_PATH)/intel64/lib-ext -L$(PIN_PATH)/extras/xed-intel64/lib -lpin -lxed $(PIN_PATH)/intel64/runtime/pincrt/crtendS.o -lpin3dwarf  -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lstdc++


BINS = wraptool.so tooltest

all: $(BINS)

wraptool.so: wraptool.cpp debug.h memtool.h ../main/pmem.h ../../lib/libwrap.a
	g++ $(PIN_C_OPTIONS) -o wraptool.o -I../main -I../../include wraptool.cpp
	g++ $(PIN_L_OPTIONS) -o wraptool.so wraptool.o ../../lib/libwrap.a -lpin -lxed -ldwarf -lelf -ldl -lrt
	
tooltest: tooltest.cpp wraptool.so debug.h wrap.h ../../lib/libwrap.a
	g++ -ggdb -o tooltest tooltest.cpp -I../../include -L../../lib -L../lib -lwrap -lpthread

clean:
	rm -f $(BINS) *.o pin.log
