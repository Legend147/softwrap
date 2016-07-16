VPATH = ../../include

OBJS = wrap.o memtool.o WrapImpl.o WrapImplUndoLog.o WrapImplSoftware.o avl.o pmem.o tracer.o virt2phys.o \
	WrapLogger.o WrapLogManager.o WrapLogManagerBlock.o AliasTable.o AliasTableHash.o AT2.o LFHT.o AT3.o ATL.o

LIB = ../../lib

LIBS = $(LIB)/libwrap.so $(LIB)/libwrap.a

all: $(LIBS)

$(LIB)/libwrap.so: $(OBJS)
	g++ -o $@ -g -shared -fPIC $^ -lpthread -lrt 

$(LIB)/libwrap.a: $(OBJS)
	ar -rv $@ $^

clean:
	rm -f $(OBJS)
	rm -f $(LIBS)

wrap.o: wrap.cpp wrap.h WrapImpl.h debug.h
	g++ $(CFLAGS) -o $@ -fPIC -c $<
	
pmem.o: pmem.cpp pmem.h
	g++ $(CFLAGS) -o $@ -fPIC -c $<
	
tracer.o: tracer.cpp tracer.h virt2phys.h
	g++ $(CFLAGS) -o $@ -fPIC -c $<
	
virt2phys.o: virt2phys.cpp virt2phys.h
	g++ $(CFLAGS) -o $@ -fPIC -c $<
	
WrapImpl.o: WrapImpl.cpp WrapImpl.h wrap.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<
	
WrapImplUndoLog.o: WrapImplUndoLog.cpp WrapImplUndoLog.h WrapImpl.h wrap.h memtool.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO) -c $<
	
WrapImplRedoLog.o: WrapImplRedoLog.cpp WrapImplRedoLog.h WrapImpl.h wrap.h memtool.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO) -c $<

WrapImplSoftware.o: WrapImplSoftware.cpp WrapImplSoftware.h WrapImpl.h wrap.h memtool.h debug.h \
		AliasTable.h AliasTableHash.h AliasTableBase.h AT2.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<
	
WrapLogger.o: WrapLogger.cpp WrapLogger.h wrap.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<
	
WrapLogManager.o: WrapLogManager.cpp WrapLogManager.h WrapLogger.h wrap.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

WrapLogManagerBlock.o: WrapLogManagerBlock.cpp WrapLogManagerBlock.h WrapLogger.h wrap.h WrapLogManager.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

memtool.o: memtool.c memtool.h debug.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO) -c $<
	
AliasTable.o: AliasTable.cpp AliasTable.h wrap.h memtool.h avl.cpp WrapLogger.h WrapLogManager.h AliasTableBase.h avl.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

AliasTableHash.o: AliasTableHash.cpp AliasTableHash.h wrap.h memtool.h LFHT.h WrapLogger.h WrapLogManager.h AliasTableBase.h avl.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

AT2.o: AT2.cpp AT2.h AliasTableHash.h wrap.h memtool.h WrapLogger.h WrapLogManager.h AliasTableBase.h avl.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

AT3.o: AT3.cpp AT3.h AliasTableHash.h wrap.h memtool.h WrapLogger.h WrapLogManager.h AliasTableBase.h avl.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

ATL.o: ATL.cpp ATL.h AliasTableHash.h wrap.h memtool.h WrapLogger.h WrapLogManager.h AliasTableBase.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

avl.o: avl.cpp avl.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<
	
LFHT.o: LFHT.cpp LFHT.h
	g++ $(CFLAGS) -o $@ -fPIC $(ATO4) -c $<

