#ifndef COMMON_H
#define COMMON_H

#include <systemc>

// ============================================================
//  ECSE 541 Assignment 3 – Shared Constants & MMIO Map
//  All teammates should #include this header.
// ============================================================

// ── System Parameters (defaults; overridden by command-line) ─
#define DEFAULT_SIZE   6
#define DEFAULT_LOOPS  1000

// Each matrix element is a 32-bit integer → 4 bytes
#define WORD_SIZE_BYTES 4

// ── Clock ────────────────────────────────────────────────────
// 150 MHz → one clock cycle = 6.667 ns
#define CLOCK_PERIOD_NS 6.667

// ── Bus Timing (in clock cycles) ────────────────────────────
//  Spec: Request ≥ 2 cc, Acknowledge 1 cc, Write 1 cc, Read 2 cc
#define BUS_REQUEST_CYCLES     2
#define BUS_ACKNOWLEDGE_CYCLES 1
#define BUS_WRITE_CYCLES       1
#define BUS_READ_CYCLES        2

// ── Bus Operation Codes ──────────────────────────────────────
#define BUS_OP_READ  0
#define BUS_OP_WRITE 1

// ── Memory-Mapped I/O Register Map ──────────────────────────
//  The first 1 KB (addresses 0x000–0x3FF) is reserved for MMIO.
//  All arrays (a, b, c) must start at or above MMIO_END.
//
//  Layout (each register is one 32-bit word = 4 bytes):
//
//  Offset  Name              Description
//  0x000   HW_CTRL           Control register (written by SW)
//                              bit 0 = START  (SW sets to 1 to trigger HW)
//                              bit 1 = RESET  (SW sets to 1 to reset HW)
//  0x004   HW_STATUS         Status register (written by HW, read by SW)
//                              bit 0 = DONE   (HW sets to 1 when finished)
//                              bit 1 = BUSY   (HW sets to 1 while running)
//  0x008   HW_ADDR_A         Base address of matrix A slice for HW
//  0x00C   HW_ADDR_B         Base address of matrix B slice for HW
//  0x010   HW_ADDR_C         Base address of matrix C element for HW
//  0x014   HW_SIZE           Matrix dimension N (NxN matrices)
//  0x018   HW_I              Current row index i  (set by SW before START)
//  0x01C   HW_J              Current col index j  (set by SW before START)
//  0x020   HW_LOOPS          Not used by HW; reserved for future use
//  0x3FF   (end of MMIO region)

#define MMIO_BASE           0x000
#define MMIO_END            0x400   // first valid array address

#define MMIO_HW_CTRL        0x000
#define MMIO_HW_STATUS      0x004
#define MMIO_HW_ADDR_A      0x008
#define MMIO_HW_ADDR_B      0x00C
#define MMIO_HW_ADDR_C      0x010
#define MMIO_HW_SIZE        0x014
#define MMIO_HW_I           0x018
#define MMIO_HW_J           0x01C

// Control register bit masks
#define HW_CTRL_START  (1u << 0)
#define HW_CTRL_RESET  (1u << 1)

// Status register bit masks
#define HW_STATUS_DONE (1u << 0)
#define HW_STATUS_BUSY (1u << 1)

// ── Default Array Base Addresses (above MMIO region) ─────────
// Arrays are laid out: C, then A, then B  (matches command-line order)
// Each NxN matrix of 32-bit words occupies SIZE*SIZE*4 bytes.
// For SIZE=6: 6*6*4 = 144 bytes per matrix; 1000 matrices → 144 000 bytes each.
//
//  Default base addresses (for LOOPS=1000, SIZE=6):
#define DEFAULT_ADDR_C  0x000400   //   1 KB
#define DEFAULT_ADDR_A  0x024400   // ~145 KB (0x400 + 0x24000)
#define DEFAULT_ADDR_B  0x048400   // ~289 KB

// ── Bus Master ID Assignments ────────────────────────────────
#define MASTER_ID_SW  0
#define MASTER_ID_HW  1

// Helper: byte address of element [n][i][j] in a matrix starting at base
// with given SIZE (elements are stored in row-major, loop-major order:
//   offset = n*SIZE*SIZE + i*SIZE + j)
inline unsigned int matrix_addr(unsigned int base,
                                unsigned int n, unsigned int i,
                                unsigned int j, unsigned int size) {
    return base + (n * size * size + i * size + j) * WORD_SIZE_BYTES;
}

#endif // COMMON_H