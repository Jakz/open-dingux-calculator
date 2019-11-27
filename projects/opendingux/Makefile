.PHONY: all clean

CXX:= $(CROSS)g++

SYSROOT:= $(shell $(CXX) -print-sysroot)
CXXFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
LDFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl-config --libs)
LDFLAGS+= -lSDL_image -SDL_ttf

CXXFLAGS+= -W -Wall -Wextra -O2 -std=c++0x

SOURCES:= $(wildcard ../../src/*.cpp)
BINARIES:= $(foreach source, $(SOURCES), $(source:%.cpp=%.o) )
EXECUTABLE:= ./lazers/lazers

all: $(EXECUTABLE)

$(EXECUTABLE): $(BINARIES)
	$(CXX) $(BINARIES) -o $@ $(LDFLAGS)
	mkdir -p data
	cp -f ../../data/* data/

#.cpp.o:
#	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) $(EXECUTABLE)