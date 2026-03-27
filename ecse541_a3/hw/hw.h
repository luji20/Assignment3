#ifndef HW_H
#define HW_H

#include <systemc>
#include "bus_if.h"
#include "common.h"

class hw_component : public sc_module, public bus_minion_if
{
public:
    sc_port<bus_master_if> bus_master_port;

    SC_HAS_PROCESS(hw_component);
    explicit hw_component(sc_module_name name);

    unsigned int mmio_incoming_write_data=0;
    sc_event mmio_write_data_ready_ev;
    sc_event mmio_ready_for_data_ev;

    // bus_minion_if - for MMIO control from SW
    void Listen(unsigned int &req_addr,
                unsigned int &req_op,
                unsigned int &req_len) override;

    void Acknowledge() override;

    void SendReadData(unsigned int data) override;

    void ReceiveWriteData(unsigned int &data){
        //mmio_ready_for_data_ev.notify();
        std::cout<<"[HW] ReceiveWriteData wwaiting  \n";
        wait(mmio_write_data_ready_ev);
        std::cout<<"[HW] ReceiveWriteData got data"<<mmio_incoming_write_data<<"\n";
        data=mmio_incoming_write_data;
    }

    void notify_request(unsigned int addr, unsigned int op, unsigned int len) {
        mmio_pending_addr = addr;
        mmio_pending_op = op;
        mmio_pending_len= len;
        mmio_request_ev.notify();
    }
    void connect_bus_ack(sc_event *ev_sw, sc_event *ev_hw){
        bus_ack_ev_sw=ev_sw;
        bus_ack_ev_hw=ev_hw;
    }

    void connect_bus_ack_done(sc_event *ev){
        bus_ack_done_ev=ev;
    }

    void connect_bus_read_done(sc_event *ev){
        bus_read_done_ev=ev;
    }
    void connect_bus_read(sc_event *ev,unsigned int *buf){
        bus_read_ev=ev;
        bus_read_buf=buf;
    }

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

    sc_event     mmio_request_ev;
    sc_event     mmio_ack_ev;
    sc_event     mmio_read_data_ev;
    sc_event     mmio_write_data_ev;
    unsigned int mmio_pending_addr = 0;
    unsigned int mmio_pending_op   = 0;
    unsigned int mmio_pending_len  = 0;
    unsigned int mmio_read_data    = 0;
    unsigned int mmio_write_data   = 0;
    sc_event *bus_ack_ev=nullptr;
    sc_event *bus_ack_ev_sw=nullptr;
    sc_event *bus_ack_ev_hw=nullptr;
    sc_event start_event;
    sc_event *bus_ack_done_ev=nullptr;
    sc_event *bus_read_done_ev=nullptr;
    sc_event *bus_read_ev=nullptr;
    unsigned int *bus_read_buf=nullptr;

    void minion_thread();   // listens for MMIO writes from SW
    void hw_thread();       // performs the k-loop offload

    unsigned int bus_read_word(unsigned int addr);
    void bus_write_word(unsigned int addr, unsigned int data);
};

#endif