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

        Listen(addr, op, len);

        Acknowledge();

        std::cout << "[MEMORY] Received " 
                  << (op == BUS_OP_READ ? "READ" : "WRITE")
                  << " @0x" << std::hex << addr 
                  << " len=" << std::dec << len << std::endl;

        if (op == BUS_OP_READ)
        {
            for (unsigned int i = 0; i < len; ++i)
            {
                unsigned int data = 0;
                unsigned int byte_addr = addr + i * WORD_SIZE_BYTES;
                if (is_valid_addr(byte_addr))
                    data = mem[byte_addr / WORD_SIZE_BYTES];
                SendReadData(data);
            }
        }
        else
        {
            for (unsigned int i = 0; i < len; ++i)
            {
                unsigned int data = 0;
                ReceiveWriteData(data);
                unsigned int byte_addr = addr + i * WORD_SIZE_BYTES;
                if (is_valid_addr(byte_addr))
                    mem[byte_addr / WORD_SIZE_BYTES] = data;
            }
        }
    }
}

void memory::Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len)
{
    wait(request_ev);
    req_addr = pending_addr;
    req_op   = pending_op;
    req_len  = pending_len;
}

void memory::Acknowledge()
{
    wait(BUS_ACKNOWLEDGE_CYCLES * sc_time(1, SC_NS));
    if(current_master==MASTER_ID_HW)
        bus_ack_ev_hw->notify();
    else
        bus_ack_ev->notify();
   bus_ack_master_ev->notify();
}

void memory::SendReadData(unsigned int data)
{
    *bus_read_data=data;
    if(bus_read_data2) *bus_read_data2=data;
    wait(BUS_READ_CYCLES * sc_time(1, SC_NS));
    std::cout<<"[MEMORY] Sending read data  "<<data<<"\n";
    bus_read_ev->notify();
    if(bus_read_ev2) bus_read_ev2->notify();
    bus_read_done_ev->notify(1,SC_NS);
}

void memory::ReceiveWriteData(unsigned int &data)
{
    std::cout<<"[MEMORY] Waiting for write data ..\n";
    wait(*bus_write_ev);
    std::cout<<"[MEMORY] Got write data ..\n";
    data = incoming_write_data;
    wait(BUS_WRITE_CYCLES * sc_time(1, SC_NS));
    // data is filled by the Bus
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