.PHONY: all clean

CXX:= $(CROSS)g++

SYSROOT:= $(shell $(CXX) -print-sysroot)
CXXFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl2-config --cflags)
LDFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl2-config --libs)
LDFLAGS+= -lSDL2_image -lSDL2_ttf

CXXFLAGS+= -W -Wall -Wextra -O2 -std=c++0x

SOURCES:= $(wildcard ../../src/*.cpp)
BINARIES:= $(foreach source, $(SOURCES), $(source:%.cpp=%.o) )
EXECUTABLE:= ./odcalc

all: $(EXECUTABLE)

$(EXECUTABLE): $(BINARIES)
	$(CXX) $(BINARIES) -o $@ $(LDFLAGS)
	mkdir -p data
	cp -f ../../data/* data/

#.cpp.o:
#	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) $(EXECUTABLE)
