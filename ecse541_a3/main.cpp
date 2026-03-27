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
    if (!parse_args(argc, argv, args)) {
        return 1;
    }

    std::cout << "=== ECSE 541 A3 – Matrix Multiplication ===\n"
              << "  mode : " << (args.hw_mode ? "HW/SW" : "pure SW") << "\n\n";

    Bus          bus("bus");
    memory       mem("memory", args.mem_file);
    hw_component hw("hw");
    sw_component sw("sw", args.addr_c, args.addr_a, args.addr_b,
                    args.size, args.loops, args.hw_mode);

    sw.bus_master_port(bus.master_export);
    hw.bus_master_port(bus.master_export);
    
    bus.connect_hw_minion(&hw);
    bus.minion_export(mem);     // Only memory is the minion
    bus.connect_minion(&mem);
    mem.connect_bus(bus.write_data_ev, bus.shared_write_data, bus.ack_ev_sw, bus.ack_done_ev, bus.read_data_ev_sw, bus.read_data_sw, bus.read_done_ev, bus.read_data_ev_hw, bus.read_data_hw, bus.ack_ev_hw);
    hw.connect_bus_ack(&bus.ack_ev_sw,&bus.ack_ev_hw);
    hw.connect_bus_ack_done(&bus.ack_done_ev);
    hw.connect_bus_read_done(&bus.read_done_ev);
    hw.connect_bus_read(&bus.read_data_ev_hw,&bus.read_data_hw);
    std::cout << "[sim] Starting simulation...\n";
    sc_start();

    std::cout << "[sim] Simulation complete.\n";
    return 0;
}