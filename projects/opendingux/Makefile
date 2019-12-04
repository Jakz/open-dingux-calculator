.PHONY: all clean

CC := $(CROSS)gcc
CXX := $(CROSS)g++

SYSROOT := $(shell $(CXX) -print-sysroot)
CXXFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl2-config --cflags)
LDFLAGS+= $(shell $(SYSROOT)/usr/bin/sdl2-config --libs)
LDFLAGS+= -lSDL2_image -lSDL2_ttf

DISABLED_WARNINGS += -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare -Wno-parentheses -Wno-unused-variable -Wno-reorder
CXXFLAGS+= -O2 -W -Wall -Wextra -std=c++11 $(DISABLED_WARNINGS) -I../../src
# -O2

SOURCES := $(wildcard ../../src/*.cpp)
SOURCES += $(wildcard ../../src/precision/*.cpp)
SOURCES += $(wildcard ../../src/views/*.cpp)
BINARIES := $(foreach source, $(SOURCES), $(source:%.cpp=%.o) )
EXECUTABLE := ./odcalc

all: $(EXECUTABLE)

$(EXECUTABLE): $(BINARIES)
	$(CXX) $(BINARIES) -o $@ $(LDFLAGS)
	mkdir -p data
	cp -f ../../data/* data/

#.cpp.o:
#	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) $(EXECUTABLE)

opk:
	rm -rf .opk_data
	mkdir -p .opk_data/data
	cp -r data/* .opk_data/data
	cp icon.png .opk_data/
	cp default.gcw0.desktop .opk_data/
	cp odcalc .opk_data/odcalc
	mksquashfs .opk_data $(EXECUTABLE).opk -all-root -noappend -no-exports -no-xattrs -no-progress > /dev/null
