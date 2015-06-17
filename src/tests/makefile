VPATH = ../../include ../../bin ../../libs

BIN = ../../bin
LIB = ../../libs

BINS = $(BIN)/testsimple $(BIN)/testarray $(BIN)/testmap $(BIN)/testwrapbtree $(BIN)/bencharray \
	$(BIN)/benchmap $(BIN)/arraytrace $(BIN)/arrayobject $(BIN)/arrayvalidate \
	$(BIN)/mtarray $(BIN)/mtarrayrate $(BIN)/simpletest $(BIN)/testwrapvar \
	$(BIN)/workerthreads $(BIN)/ssarray $(BIN)/ssarrayna

all: $(BINS)

clean:
	rm -f $(BINS)

$(BIN)/testsimple: testsimple.cpp wrap.h
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap -lpthread
	
$(BIN)/mtarray: mtarray.cpp WrapArray.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap -lpthread

$(BIN)/mtarrayrate: mtarrayrate.cpp wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap -lpthread

$(BIN)/workerthreads: workerthreads.cpp wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap -lpthread

$(BIN)/testarray: testarray.cpp WrapArray.h WrapVar.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/testwrapvar: testwrapvar.cpp WrapVar.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/testwrapbtree: testwrapbtree.cpp WrapVar.h WrapBTree.h wrap.h libwrap.so
	g++ $(CFLAGS) $(ATO4) -o $@ $< -L$(LIB) -lwrap 
	#-DBTREE_DEBUG

$(BIN)/testmap: testmap.cpp WrapMap.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap
	
$(BIN)/bencharray: bencharray.cpp WrapArray.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/benchmap: benchmap.cpp WrapMap.h wrap.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap
	
$(BIN)/arraytrace: arraytrace.cpp wrap.h memtool.h tracer.h libwrap.so 
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/ssarray: ssarray.cpp wrap.h memtool.h tracer.h libwrap.so 
	g++ $(CFLAGS) $(ATO) -o $@ $< -L$(LIB) -lwrap

$(BIN)/ssarrayna: ssarray.cpp wrap.h memtool.h tracer.h libwrap.so 
	g++ $(CFLAGS) $(ATO) -DNA -o $@ $< -L$(LIB) -lwrap

$(BIN)/arrayvalidate: arrayvalidate.cpp wrap.h memtool.h tracer.h libwrap.so 
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/arrayobject: arrayobject.cpp wrap.h memtool.h tracer.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap

$(BIN)/simpletest: simpletest.cpp wrap.h memtool.h tracer.h libwrap.so
	g++ $(CFLAGS) -o $@ $< -L$(LIB) -lwrap
	