# ECSE 541 - Assignment 3: Computer System Design in SystemC

**Title:** HW/SW Co-Design of Matrix Multiplication using SystemC with Custom Bus

**Group Members:**
- Abhishree (Software Component + Command-line Parsing)
- Lucero (Memory Component + Golden Model Support)
- Jacob (Hardware Coprocessor)
- Maninder (Bus Specialist)

## 1. Project Overview

This project implements a cycle-accurate SystemC model of a heterogeneous system that performs matrix multiplication with optional hardware acceleration of the innermost k-loop.

### Key Features
- 150 MHz clock domain
- Custom TLM-style bus with round-robin arbitration between SW and HW masters
- Memory-mapped I/O (MMIO) region for HW control
- Hardware coprocessor that offloads the k-loop
- Pure software fallback mode
- Cycle-accurate performance modeling
- Golden C reference model for verification
- Support for both small and large test cases

## 2. System Architecture

- **Software Component**: Handles outer loops (n, i, j), initializes matrices, triggers HW via MMIO, and collects results.
- **Hardware Coprocessor**: Accelerates the innermost k-loop using bus master transactions.
- **Memory Module**: Single shared memory with validation and MMIO protection.
- **Bus Module**: Central interconnect with round-robin arbitration, single and burst support, and accurate cycle delays.
- **Golden Model**: Pure C implementation for functional verification.

## 3. File Structure
ecse541_a3/
├── main.cpp                 # Top-level sc_main and module binding
├── Makefile
├── README.md
├── test_2x2.mem             # Small 2x2 test case
├── mem_init.txt             # Large test data (optional)
├── golden_model.c           # Golden reference
├── mm                       # Main SystemC executable
├── golden_mm                # Golden model executable
├── bus/
│   ├── bus.h
│   └── bus.cpp              
├── sw/
│   ├── sw.h
│   ├── sw.cpp
│   └── cmd_args.h
├── hw/
│   ├── hw.h
│   └── hw.cpp
├── mem/
│   ├── memory.h
│   └── memory.cpp
└── shared/
├── common.h             # Constants, MMIO map, helpers
└── bus_if.h             # Master and Minion interfaces

## 4. How to Build

```bash
make clean
make

This builds:

./mm           // Full SystemC simulation (HW/SW or pure SW)
./golden_mm    // Golden C reference model

## 5. How to Run

# Small test case

# HW/SW Co-design mode
./mm test_2x2.mem 0x400 0x410 0x420 2 1

# Pure Software mode
./mm test_2x2.mem 0x400 0x410 0x420 2 0

# Large Test case
./mm mem_init.txt 0x400 0x024400 0x048400 6 1

# Golden Model Verification
./golden_mm test_2x2.mem 0x400 0x410 0x420 2 1

## 6. Command Line Format
- Bash./mm <memory_file> <addrC> <addrA> <addrB> <size> <hw_enabled>
- Parameters:
    - memory_file : Path to memory initialization file
    - addrC, addrA, addrB : Base addresses (in bytes) for matrices C, A, B
    - size : Matrix dimension N (default 6)
    - hw_enabled : 1 = enable HW acceleration, 0 = pure software

## 7. Expected Output

=== ECSE 541 A3 – Matrix Multiplication ===
  mode : HW/SW
[sim] Starting simulation...
[HW] HW component started
[SW] Starting. HW enabled: yes
[BUS] Master 0 requested @0x400 len=1
[BUS] Granted to master 0
[MEMORY] Received WRITE request @0x400 len=1
[BUS] Acknowledge sent
... (multiple bus transactions)
[HW] START command received - starting k-loop
[HW] k-loop finished, result written to 0x...
[sim] Simulation complete.

## 8. Performance Comparison
**After successful run, record:**

- Pure SW execution time (cycles)
- HW/SW co-design execution time (cycles)
- Speedup factor
- Bus utilization breakdown