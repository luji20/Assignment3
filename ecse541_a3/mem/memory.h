#ifndef MEMORY_H
#define MEMORY_H

#include <systemc>
#include "common.h"
#include "bus_if.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace sc_core;

class memory : public sc_module, public bus_minion_if
{
public:
    SC_HAS_PROCESS(memory);

    memory(sc_module_name name, const std::string &memfile);

    unsigned int incoming_write_data=0;

    void Listen(unsigned int &req_addr,
                unsigned int &req_op,
                unsigned int &req_len) override;

    void Acknowledge() override;

    void SendReadData(unsigned int data) override;

    void ReceiveWriteData(unsigned int &data) override;

    void connect_bus(sc_event &wev, unsigned int &wbuf,sc_event &aev,sc_event &amev,sc_event &rev, unsigned int &rbuf,sc_event &rdev,sc_event &rev2, unsigned int &rbuf2,sc_event &aev_hw) {
        bus_write_ev   = &wev;
        bus_write_data = &wbuf;
        bus_ack_ev=&aev;
        bus_ack_master_ev= &amev;
        bus_read_ev   = &rev;
        bus_read_data = &rbuf;
        bus_read_ev2   = &rev2;
        bus_read_data2 = &rbuf2;
        bus_read_done_ev = &rdev;
        bus_ack_ev_hw= &aev_hw;
    }

    void notify_request(unsigned int addr,unsigned int op, unsigned int len){
        pending_addr=addr;
        pending_op=op;
        pending_len=len;
        request_ev.notify();
    }
    void set_current_master(unsigned int mid){
        current_master=mid;
    }

private:
    std::vector<unsigned int> mem;

    sc_event request_ev;
    sc_event ack_ev;
    sc_event data_ev;
    sc_event *bus_ack_ev=nullptr;
    sc_event *bus_ack_master_ev=nullptr;

    sc_event     *bus_write_ev   = nullptr;
    unsigned int *bus_write_data = nullptr;
    sc_event     *bus_read_ev   = nullptr;
    unsigned int *bus_read_data = nullptr;
    sc_event     *bus_read_ev2   = nullptr;
    unsigned int *bus_read_data2 = nullptr;
    sc_event     *bus_read_done_ev   = nullptr;

    unsigned int current_master=0;
    sc_event *bus_ack_ev_hw=nullptr;
    unsigned int pending_addr = 0;
    unsigned int pending_op   = 0;
    unsigned int pending_len  = 0;

    void run();

    bool is_valid_addr(unsigned int addr) const;

    void load_file(const std::string &memfile);
};

#endif