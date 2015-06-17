#  Make options include
#
#   debug=1
#   sim=1 // builds the simulator and pin
#

export PIN_PATH = $(HOME)/pin
export INCLUDE := $(PWD)/../include

export CFLAGS = -Wall -Werror -msse2 -I$(INCLUDE)

ifeq ($(debug),1)
	export CFLAGS += -g
else
	export ATO = -O
	export ATO4 = -O4
endif

ifeq ($(opt),1)
export CFLAGS += -DNDEBUG -O
endif

ifeq ($(tracer),1)
export CFLAGS += -DTRACER -DNDEBUG
endif

all:
	$(MAKE) -C main
	$(MAKE) -C tests
	$(MAKE) -C tracer
ifeq ($(sim),1)
	$(MAKE) -C simulator
endif
	
clean:
	$(MAKE) -C main clean
	$(MAKE) -C tests clean
	$(MAKE) -C tracer clean
	$(MAKE) -C simulator clean

runtests: $(BINS)
	echo "Basic Tests"
	./bencharray 100 100 100 0
	./bencharray 100 100 100 1
	./bencharray 100 100 100 2
	./bencharray 100 100 100 3
	./bencharray 100 100 100 4
	$(PIN) -t wraptrace3.so -assertwrap -- ./testsimple 10
	$(PIN) -t wraptrace3.so -assertwrap -- ./testmap
	$(PIN) -t wraptrace3.so -assertwrap -- ./testarray
	$(PIN) -t wraptrace3.so -assertwrap -- ./bencharray 100 100 100
	$(PIN) -t wraptrace3.so -assertwrap -- ./benchmap 100 100 100
	$(PIN) -t wraptrace3.so -assertwrap -- ./arraytrace 1 100 100
	$(PIN) -t wraptrace3.so -assertwrap -- ./arraytrace 0 100 100		
    


# DO NOT DELETE
