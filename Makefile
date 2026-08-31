PRESET ?= desktop-debug 

.PHONY: all clean build clean rebuild

all: build

configure: 
	cmake --preset=$(PRESET)

build: 
	cmake --preset=$(PRESET)
	cmake --build --preset=$(PRESET)

clean:
	cmake --build --preset=$(PRESET) --target clean

rebuild: clean configure build
