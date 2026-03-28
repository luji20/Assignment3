// mem/memory.cpp
#include "memory.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

memory::memory(sc_module_name name, const std::string &memfile)
    : sc_module(name)
{
    load_file(memfile);
    SC_THREAD(run);
}

void memory::run()
{
    while (true)
    {
        unsigned int addr = 0, op = 0, len = 0;
        bus_minion_port->Listen(addr, op, len);

        // Ignore requests meant for HW MMIO registers
        if (addr < MMIO_END) {
            continue; 
        }

        bus_minion_port->Acknowledge();

        if (op == BUS_OP_READ) {
            for (unsigned int i = 0; i < len; ++i) {
                unsigned int data = 0;
                unsigned int byte_addr = addr + i * WORD_SIZE_BYTES;
                if (is_valid_addr(byte_addr))
                    data = mem[byte_addr / WORD_SIZE_BYTES];
                bus_minion_port->SendReadData(data);
            }
        }
        else {
            for (unsigned int i = 0; i < len; ++i) {
                unsigned int data = 0;
                bus_minion_port->ReceiveWriteData(data);
                unsigned int byte_addr = addr + i * WORD_SIZE_BYTES;
                if (is_valid_addr(byte_addr))
                    mem[byte_addr / WORD_SIZE_BYTES] = data;
            }
        }
    }
}

bool memory::is_valid_addr(unsigned int addr) const
{
    return (addr / WORD_SIZE_BYTES) < mem.size();
}

void memory::load_file(const std::string &memfile)
{
    mem.assign(200000, 0);

    std::ifstream f(memfile);
    if (!f) {
        std::cerr << "[memory] WARNING: cannot open file\n";
        return;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        unsigned int addr, val;
        if (sscanf(line.c_str(), "%x %x", &addr, &val) == 2) {
            unsigned int idx = addr / WORD_SIZE_BYTES;
            if (idx < mem.size()) mem[idx] = val;
        }
    }
}