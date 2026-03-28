#ifndef CMD_ARGS_H
#define CMD_ARGS_H

// ============================================================
//  ECSE 541 Assignment 3 – Command-Line Argument Parser
//  Author: Abhishree
//
//  Parses:
//    mm <memoryInitFile> [[[addrC addrA addrB] size] loops]
//
//  Rules from the spec:
//    • addrC, addrA, addrB must all be supplied together or not at all
//    • size and loops are individually optional after the addresses
//    • All addresses must lie above MMIO_END (0x400)
// ============================================================

#include <string>
#include <iostream>
#include <cstdlib>
#include "common.h"

struct ProgramArgs {
    std::string  mem_file;               // path to memory init file
    unsigned int addr_c   = DEFAULT_ADDR_C;
    unsigned int addr_a   = DEFAULT_ADDR_A;
    unsigned int addr_b   = DEFAULT_ADDR_B;
    unsigned int size     = DEFAULT_SIZE;
    unsigned int loops    = DEFAULT_LOOPS;
    bool         hw_mode  = false;
    // default: run HW/SW partition
};

// ── Print usage to stderr ─────────────────────────────────────
inline void print_usage(const char *prog)
{
    std::cerr << "Usage: " << prog
              << " <memInitFile> [[[addrC addrA addrB] size] loops]\n"
              << "\n"
              << "  memInitFile  path to binary memory initialisation file\n"
              << "  addrC        base address of matrix C (hex, e.g. 0x000400)\n"
              << "  addrA        base address of matrix A\n"
              << "  addrB        base address of matrix B\n"
              << "  size         matrix dimension (default " << DEFAULT_SIZE << ")\n"
              << "  loops        number of matrix multiplications (default "
              << DEFAULT_LOOPS << ")\n"
              << "\n"
              << "  All three addresses must be above MMIO region [0x000, 0x"
              << std::hex << MMIO_END << std::dec << ").\n";
}

// ── Validate one address is above the MMIO region ─────────────
inline bool check_address(unsigned int addr, const char *name)
{
    if (addr < MMIO_END) {
        std::cerr << "Error: " << name << " (0x" << std::hex << addr
                  << ") overlaps MMIO region [0x000, 0x" << MMIO_END
                  << ").\n" << std::dec;
        return false;
    }
    return true;
}

// ── Parse argc/argv → ProgramArgs ────────────────────────────
//  Returns true on success; false if arguments are invalid.
inline bool parse_args(int argc, char *argv[], ProgramArgs &out)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }

    out.mem_file = argv[1];

    // Groups: addresses (3 args), size (1 arg), loops (1 arg)
    // Per spec: "[[[addrC addrA addrB] size] loops]"
    // i.e. loops requires size, size requires all three addresses.

    if (argc >= 5) {
        // All three addresses supplied
        out.addr_c = static_cast<unsigned int>(std::strtoul(argv[2], nullptr, 0));
        out.addr_a = static_cast<unsigned int>(std::strtoul(argv[3], nullptr, 0));
        out.addr_b = static_cast<unsigned int>(std::strtoul(argv[4], nullptr, 0));

        if (!check_address(out.addr_c, "addrC")) return false;
        if (!check_address(out.addr_a, "addrA")) return false;
        if (!check_address(out.addr_b, "addrB")) return false;
    } else if (argc >= 3 && argc < 5) {
        // Partial address group — not allowed
        std::cerr << "Error: addrC, addrA, and addrB must all be supplied together.\n";
        print_usage(argv[0]);
        return false;
    }

    if (argc >= 6) {
        out.size = static_cast<unsigned int>(std::strtoul(argv[5], nullptr, 0));
        if (out.size == 0) {
            std::cerr << "Error: size must be > 0.\n";
            return false;
        }
    }

    if (argc >= 7) {
        out.loops = static_cast<unsigned int>(std::strtoul(argv[6], nullptr, 0));
        if (out.loops == 0) {
            std::cerr << "Error: loops must be > 0.\n";
            return false;
        }
    }

    // ── Overlap check between arrays ─────────────────────────
    // Each matrix occupies loops * size * size * WORD_SIZE_BYTES bytes.
    unsigned int matrix_bytes = out.loops * out.size * out.size * WORD_SIZE_BYTES;

    auto overlaps = [&](unsigned int base1, const char *n1,
                         unsigned int base2, const char *n2) -> bool {
        if (base1 < base2 + matrix_bytes && base2 < base1 + matrix_bytes) {
            std::cerr << "Error: " << n1 << " and " << n2 << " overlap in memory.\n";
            return true;
        }
        return false;
    };

    if (overlaps(out.addr_c, "addrC", out.addr_a, "addrA")) return false;
    if (overlaps(out.addr_c, "addrC", out.addr_b, "addrB")) return false;
    if (overlaps(out.addr_a, "addrA", out.addr_b, "addrB")) return false;

    return true;
}

#endif // CMD_ARGS_H
