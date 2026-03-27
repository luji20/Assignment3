#ifndef BUS_H
#define BUS_H

#include <systemc>
#include "bus_if.h"
#include "../mem/memory.h"
#include "../hw/hw.h"
#include "common.h"

using namespace sc_core;

#define BUS_DEBUG 1

#if BUS_DEBUG
  #define BUS_PRINT(msg) std::cout << sc_time_stamp() << " [BUS] " << msg << std::endl
#else
  #define BUS_PRINT(msg) do {} while(0)
#endif

struct PendingRequest {
    unsigned int master_id = 0;
    unsigned int address   = 0;
    unsigned int operation = 0;
    unsigned int length    = 0;
    bool         active    = false;
};

class Bus : public sc_module,
            public bus_master_if,
            public bus_minion_if
{
public:
    sc_export<bus_master_if> master_export;
    sc_export<bus_minion_if> minion_export;

    unsigned int shared_read_data  = 0;
    sc_event read_data_ev;
    sc_event read_done_ev;
    sc_event ack_done_ev;
    sc_event ack_ev;
    sc_event ack_ev_sw;
    sc_event ack_ev_hw;
    sc_event hw_ack_ev;


    sc_event write_data_ev;
    sc_event read_data_ev_sw;
    sc_event read_data_ev_hw;
    unsigned int shared_write_data = 0;
    unsigned int read_data_sw = 0;
    unsigned int read_data_hw = 0;
    unsigned int last_ack_master = 0;

    SC_HAS_PROCESS(Bus);

    explicit Bus(sc_module_name name);

    // bus_master_if
    void Request(unsigned int mst_id, unsigned int addr,
                 unsigned int op, unsigned int len) override;

    bool WaitForAcknowledge(unsigned int mst_id) override;

    void ReadData(unsigned int &data) override;

    void WriteData(unsigned int data) override;

    // bus_minion_if
    void Listen(unsigned int &req_addr,
                unsigned int &req_op,
                unsigned int &req_len) override;

    void Acknowledge() override;

    void SendReadData(unsigned int data) override;

    void ReceiveWriteData(unsigned int &data) override;

    void connect_minion(memory *m){minion_mem=m;}
    void connect_hw_minion(hw_component *hw){hw_minion=hw;}

private:
    PendingRequest current_req;
    PendingRequest pending[2];
    unsigned int   rr_next = 0;
    memory *minion_mem=nullptr;
    hw_component *hw_minion=nullptr;

    sc_event request_ev;


    void arbiter_thread();
    void data_thread();

    int select_next_master();
    void grant_to_master(int mid);
};

#endif