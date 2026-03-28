# ECSE 541 - Assignment 3: Matrix Multiplication Co-Design

A cycle-accurate SystemC model of a heterogeneous system performing matrix multiplication with an optional hardware coprocessor for accelerating the innermost k-loop.

## Makefile Setup

Create a `Makefile` in the root of the project with the following basic configuration. Ensure you set the correct path to your SystemC installation (`SYSTEMC_HOME`):
CXX = g++

CXXFLAGS = -std=gnu++17 -Wall -g \
           -I<SYSTEMC_HOME>/include \
           -I./shared \
           -I./sw \
           -I./mem \
           -I./hw \
           -I.bus \
           -I./golden

LDFLAGS = -L<SYSTEMC_HOME>/lib \
          -Wl,-rpath,<SYSTEMC_HOME>/lib \
          -lsystemc -lm

# ── Targets ───────────────────────────────────────────────────
.PHONY: all sim golden clean

all: sim golden

# SystemC simulation
sim: main.cpp sw/sw.cpp hw/hw.cpp bus/bus.cpp mem/memory.cpp
	$(CXX) $(CXXFLAGS) -o mm $^ $(LDFLAGS)

# Pure-C golden model
golden: golden_model.c
	gcc -O0 -I./shared -o golden_mm golden_model.c

clean:
	rm -f mm golden_mm *.o


## Build Instructions

Compile the SystemC simulation and the Golden C model using:
```bash
make 
```

## Running the Application

**Usage (`./mm <memoryInitFile> [[[addrC addrA addrB] size] loops]`):**
**Note: addresses must be larger than 0x400**
```bash
# Run HW/SW Co-design mode (Small 2x2 test case)
./mm test_large.mem [[[addrC addrA addrB] size] loops]

# Run SW for comparison
# Change bool hw_mode = false; in sw/cmd_args.h and make
./mm test_large.mem [[[addrC addrA addrB] size] loops]

# To condense output add at the end of the prompt:
 2>/dev/null | grep -E "k-loop finished|SW|Done"
```