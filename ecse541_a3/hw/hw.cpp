#include "hw.h"
#include <iostream>

hw_component::hw_component(sc_module_name name)
    : sc_module(name)
{
    SC_THREAD(minion_thread);
    SC_THREAD(hw_thread);
}

// ------------------------------------------------------------------
// Minion thread - listens for MMIO writes from SW via the Bus
// ------------------------------------------------------------------
void hw_component::minion_thread()
{
    while (true)
    {
        unsigned int addr, op, len;
        std::cout<<"[HW minion] waitingg for req \n";
        Listen(addr, op, len);
        std::cout<<"[HW minion] got req"<<std::hex<<addr<<"op="<<op<<"\n";

        Acknowledge();
        std::cout<<"[HW minion]acknoleged \n";
        std::cout<<"[HW minion] op"<<op<<"len="<<len<<"BUS_OP_WRITE="<<BUS_OP_WRITE<<"\n";
        if (op == BUS_OP_WRITE && len == 1)
        {
            std::cout<<"[HW Minion] enteriing  wwrite \n";
            unsigned int data = 0;
            ReceiveWriteData(data);

            switch (addr)
            {
                case MMIO_HW_CTRL:
                    ctrl = data;
                    if (ctrl & HW_CTRL_RESET){
                        status=0;
                        std::cout << "[HW] RESET  received - status cleared\n";
                    }
                    if (ctrl & HW_CTRL_START) {
                        start_event.notify();
                        status |= HW_STATUS_BUSY;
                        std::cout << "[HW] START command received - starting k-loop\n";
                    }
                    break;

                case MMIO_HW_ADDR_A:  addr_a = data; break;
                case MMIO_HW_ADDR_B:  addr_b = data; break;
                case MMIO_HW_ADDR_C:  addr_c = data; break;
                case MMIO_HW_SIZE:    size   = data; break;
                case MMIO_HW_I:       i      = data; break;
                case MMIO_HW_J:       j      = data; break;

                default:
                    std::cout << "[HW] Warning: write to unknown MMIO 0x" << std::hex << addr << std::endl;
            }
        }
        else if (op == BUS_OP_READ && len == 1) {
            unsigned int val = (addr == MMIO_HW_STATUS) ? status : 0;
            SendReadData(val);
        }
    }
}

// ------------------------------------------------------------------
// HW thread - performs the actual k-loop offload using bus master port
// ------------------------------------------------------------------
void hw_component::hw_thread()
{
    while (true)
    {
        wait(start_event);

        std::cout << "[HW] Starting k-loop offload for i=" << i << " j=" << j << std::endl;

        int acc = 0;
        for (unsigned int k = 0; k < size; ++k)
        {
            unsigned int a_val = bus_read_word(addr_a + k * WORD_SIZE_BYTES);
            std::cout<<"[HW] k= "<<k<< "a_val"<<a_val<<"now reading b \n";
            unsigned int b_val = bus_read_word(addr_b + k * size * WORD_SIZE_BYTES);
            std::cout<<"[HW] k= "<<k<< "b_val"<<b_val<<"\n";
            acc += (int)a_val * (int)b_val;
        }

        bus_write_word(addr_c, (unsigned int)acc);

        // Signal done
        status |= HW_STATUS_DONE;
        status &= ~HW_STATUS_BUSY;

        std::cout << "[HW] k-loop finished, result written to 0x" << std::hex << addr_c 
                  << " = " << acc << std::endl;
    }
}

// ------------------------------------------------------------------
// Bus helper functions (master side)
// ------------------------------------------------------------------
unsigned int hw_component::bus_read_word(unsigned int addr)
{
    wait(BUS_REQUEST_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    bus_master_port->Request(MASTER_ID_HW, addr, BUS_OP_READ, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_HW);
    //wait(BUS_READ_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    unsigned int data = 0;
    bus_master_port->ReadData(data);
    return data;
}

void hw_component::bus_write_word(unsigned int addr, unsigned int data)
{
    wait(BUS_REQUEST_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    bus_master_port->Request(MASTER_ID_HW, addr, BUS_OP_WRITE, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_HW);
    wait(BUS_WRITE_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));

    bus_master_port->WriteData(data);
}

// ------------------------------------------------------------------
// bus_minion_if stubs (required because we inherit from it)
// ------------------------------------------------------------------
void hw_component::Listen(unsigned int &req_addr,
                          unsigned int &req_op,
                          unsigned int &req_len)
{
    wait(mmio_request_ev);
    req_addr = mmio_pending_addr;
    req_op   = mmio_pending_op;
    req_len  = mmio_pending_len;}

void hw_component::Acknowledge() {
    //wait(BUS_ACKNOWLEDGE_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    mmio_ack_ev.notify();
    if(bus_ack_ev_sw) bus_ack_ev_sw->notify();
    if(bus_ack_done_ev) bus_ack_done_ev->notify();
}
void hw_component::SendReadData(unsigned int data) {
    //mmio_read_data = data;
    if(bus_read_buf) *bus_read_buf=data;
    wait(BUS_READ_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    if(bus_read_ev) bus_read_ev->notify();
    mmio_read_data_ev.notify();
    if(bus_read_done_ev)bus_read_done_ev->notify();
}
