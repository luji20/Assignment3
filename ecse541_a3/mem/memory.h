#ifndef MEMORY_H
#define MEMORY_H

#include <systemc>
#include "common.h"
#include "bus_if.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace sc_core;

class memory : public sc_module
{
public:
    sc_port<bus_minion_if> bus_minion_port; // Connecting to bus
    SC_HAS_PROCESS(memory);
    memory(sc_module_name name, const std::string &memfile);

private:
    std::vector<unsigned int> mem;
    void run();
    bool is_valid_addr(unsigned int addr) const;
    void load_file(const std::string &memfile);
};

#endif