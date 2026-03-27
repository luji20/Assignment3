#ifndef SW_H
#define SW_H

#include <systemc>
#include "bus_if.h"
#include "common.h"

SC_MODULE(sw_component)
{
    sc_port<bus_master_if> bus_master_port;

    SC_HAS_PROCESS(sw_component);

    sw_component(sc_module_name name,
                 unsigned int addr_c,
                 unsigned int addr_a,
                 unsigned int addr_b,
                 unsigned int size,
                 unsigned int loops,
                 bool hw_enabled);

    sc_time get_total_time() const { return m_total_time; }

private:
    unsigned int m_addr_c, m_addr_a, m_addr_b, m_size, m_loops;
    bool m_hw_enabled;
    sc_time m_total_time;

    void sw_thread();

    unsigned int bus_read_word(unsigned int addr);
    void bus_write_word(unsigned int addr, unsigned int data);

    void hw_trigger_and_wait(unsigned int n, unsigned int i, unsigned int j);

    void wait_cycles(unsigned int cycles);

    unsigned int elem_addr(unsigned int base, unsigned int n, unsigned int i, unsigned int j) const;
};

#endif