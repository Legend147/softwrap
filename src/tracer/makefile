VPATH = ../../include ../../bin ../../libs

BIN = ../../bin
LIB = ../../libs

BINS = $(BIN)/virtualtest $(BIN)/virt2phystest $(BIN)/virt2actual $(BIN)/phys2actual $(BIN)/physicaltracer

all: $(BINS)

clean:
	rm -f $(BINS)

#TODO organize and refactor virt2phys space from main wrap

$(BIN)/virt2phystest: virt2phystest.cpp 
	g++ $(CFLAGS) -I../main -o $@ $^ -lpthread -lrt -L$(LIB) -lwrap

$(BIN)/virt2actual: virt2actual.cpp 
	g++ $(CFLAGS) -I../main -o $@ $^ -lpthread -lrt -L$(LIB) -lwrap

$(BIN)/phys2actual: phys2actual.cpp 
	g++ $(CFLAGS) -I../main -o $@ $^ -lpthread -lrt -L$(LIB) -lwrap

$(BIN)/physicaltracer: physicaltracer.cpp 
	g++ $(CFLAGS) -I../main -o $@ $^ -lpthread -lrt -L$(LIB) -lwrap
	
$(BIN)/virtualtest: virtualtest.c
	g++ $(CFLAGS) -o $@ $^
