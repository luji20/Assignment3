#include "sw.h"
#include <iostream>
#include <iomanip>

sw_component::sw_component(sc_module_name name,
                           unsigned int addr_c,
                           unsigned int addr_a,
                           unsigned int addr_b,
                           unsigned int size,
                           unsigned int loops,
                           bool hw_enabled)
    : sc_module(name),
      m_addr_c(addr_c), m_addr_a(addr_a), m_addr_b(addr_b),
      m_size(size), m_loops(loops), m_hw_enabled(hw_enabled),
      m_total_time(SC_ZERO_TIME)
{
    SC_THREAD(sw_thread);
}

unsigned int sw_component::elem_addr(unsigned int base,
                                     unsigned int n,
                                     unsigned int i,
                                     unsigned int j) const
{
    return matrix_addr(base, n, i, j, m_size);
}

void sw_component::wait_cycles(unsigned int cycles)
{
    sc_time delta(CLOCK_PERIOD_NS * cycles, SC_NS);
    m_total_time += delta;
    wait(delta);
}

unsigned int sw_component::bus_read_word(unsigned int addr)
{
    wait_cycles(BUS_REQUEST_CYCLES);
    bus_master_port->Request(MASTER_ID_SW, addr, BUS_OP_READ, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_SW);
    //wait_cycles(BUS_READ_CYCLES);
    unsigned int data = 0;
    bus_master_port->ReadData(data);
    return data;
}

void sw_component::bus_write_word(unsigned int addr, unsigned int data)
{
    wait_cycles(BUS_REQUEST_CYCLES);
    bus_master_port->Request(MASTER_ID_SW, addr, BUS_OP_WRITE, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_SW);
    wait_cycles(BUS_WRITE_CYCLES);
    bus_master_port->WriteData(data);
}

void sw_component::hw_trigger_and_wait(unsigned int n, unsigned int i, unsigned int j)
{
    bus_write_word(MMIO_HW_CTRL, HW_CTRL_RESET);
    bus_write_word(MMIO_HW_CTRL, 0);

    bus_write_word(MMIO_HW_ADDR_A, elem_addr(m_addr_a, n, i, 0));
    bus_write_word(MMIO_HW_ADDR_B, elem_addr(m_addr_b, n, 0, j));
    bus_write_word(MMIO_HW_ADDR_C, elem_addr(m_addr_c, n, i, j));
    bus_write_word(MMIO_HW_SIZE, m_size);
    bus_write_word(MMIO_HW_I, i);
    bus_write_word(MMIO_HW_J, j);

    bus_write_word(MMIO_HW_CTRL, HW_CTRL_START);

    unsigned int status = 0;
    do {
        status = bus_read_word(MMIO_HW_STATUS);
    } while (!(status & HW_STATUS_DONE));

    bus_write_word(MMIO_HW_CTRL, 0);
}

void sw_component::sw_thread()
{
    std::cout << "[SW] Starting. HW enabled: " << (m_hw_enabled ? "yes" : "no") << "\n";

    for (unsigned int n = 0; n < m_loops; n++) {
        for (unsigned int i = 0; i < m_size; i++) {
            for (unsigned int j = 0; j < m_size; j++) {
                bus_write_word(elem_addr(m_addr_c, n, i, j), 0);
            }
        }

        for (unsigned int i = 0; i < m_size; i++) {
            for (unsigned int j = 0; j < m_size; j++) {
                if (m_hw_enabled) {
                    hw_trigger_and_wait(n, i, j);
                } else {
                    int acc = 0;
                    for (unsigned int k = 0; k < m_size; k++) {
                        unsigned int a_val = bus_read_word(elem_addr(m_addr_a, n, i, k));
                        unsigned int b_val = bus_read_word(elem_addr(m_addr_b, n, k, j));
                        acc += (int)a_val * (int)b_val;
                    }
                    bus_write_word(elem_addr(m_addr_c, n, i, j), (unsigned int)acc);
                }
            }
        }
    }

    std::cout << "[SW] Done.\n";
}