#ifndef HW_H
#define HW_H

#include <systemc>
#include "bus_if.h"
#include "common.h"

class hw_component : public sc_module
{
public:
    sc_port<bus_master_if> bus_master_port;
    sc_port<bus_minion_if> bus_minion_port; // Connecting to bus

    SC_HAS_PROCESS(hw_component);
    explicit hw_component(sc_module_name name);

private:
    // MMIO registers
    unsigned int ctrl   = 0;
    unsigned int status = 0;
    unsigned int addr_a = 0;
    unsigned int addr_b = 0;
    unsigned int addr_c = 0;
    unsigned int size   = 0;
    unsigned int i      = 0;
    unsigned int j      = 0;

    sc_event start_event;

    void minion_thread();   
    void hw_thread();       

    unsigned int bus_read_word(unsigned int addr);
    void bus_write_word(unsigned int addr, unsigned int data);
};

#endif