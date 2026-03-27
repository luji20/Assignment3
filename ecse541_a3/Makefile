CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -I. -Ishared -I/opt/systemc/include
LDFLAGS = -L/opt/systemc/lib-linux64 -lsystemc -lm

all: mm golden_mm

mm: main.cpp sw/sw.cpp hw/hw.cpp mem/memory.cpp bus/bus.cpp
	$(CXX) $(CXXFLAGS) $^ -o mm $(LDFLAGS)

# Golden model (works whether the file is in root or in golden/ folder)
golden_mm: golden_model.c
	gcc -O0 -o golden_mm golden_model.c

clean:
	rm -f mm golden_mm *.o