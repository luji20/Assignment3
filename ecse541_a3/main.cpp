// main.cpp
#include <systemc>
#include <iostream>
#include "shared/common.h"
#include "shared/bus_if.h"
#include "sw/cmd_args.h"
#include "sw/sw.h"
#include "hw/hw.h"
#include "mem/memory.h"
#include "bus/bus.h"

int sc_main(int argc, char *argv[])
{
    ProgramArgs args;
    if (!parse_args(argc, argv, args)) return 1;

    std::cout << "=== ECSE 541 A3 – Matrix Multiplication ===\n"
              << "  mode : " << (args.hw_mode ? "HW/SW" : "pure SW") << "\n\n";

    Bus          bus("bus");
    memory       mem("memory", args.mem_file);
    hw_component hw("hw");
    sw_component sw("sw", args.addr_c, args.addr_a, args.addr_b,
                    args.size, args.loops, args.hw_mode);

    // Master connections
    sw.bus_master_port(bus.master_export);
    hw.bus_master_port(bus.master_export);
    
    // Minion connections (both bind to the same export seamlessly)
    hw.bus_minion_port(bus.minion_export);
    mem.bus_minion_port(bus.minion_export);

    std::cout << "[sim] Starting simulation...\n";
    sc_start();

    std::cout << "[sim] Simulation complete.\n";
    return 0;
}