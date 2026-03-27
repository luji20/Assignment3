#include "bus.h"
#include "../mem/memory.h"

Bus::Bus(sc_module_name name)
    : sc_module(name),
      master_export("master_export"),
      minion_export("minion_export")
{
    master_export(*this);

    SC_THREAD(arbiter_thread);
    SC_THREAD(data_thread);

    for (int i = 0; i < 2; ++i) pending[i].active = false;
}

void Bus::Request(unsigned int mst_id, unsigned int addr,
                  unsigned int op, unsigned int len)
{
    if (mst_id >= 2) return;
    if (pending[mst_id].active) return;

    pending[mst_id] = {mst_id, addr, op, len, true};
    BUS_PRINT("Master " << mst_id << " requested @0x" << std::hex << addr << " len=" << len);
    request_ev.notify();
}

bool Bus::WaitForAcknowledge(unsigned int mst_id)
{
    while (true) {
        if(mst_id==MASTER_ID_SW){
            wait(ack_ev_sw);
        } else {
            wait(ack_ev_hw);
        }
        //std:: cout<< "[BUS] ACK received, current_master id"<<current_req.master_id<<"waiting for "<<mst_id<<"\n";
        last_ack_master=mst_id;
        BUS_PRINT("Master " << mst_id << " received ACK");
        return true;
    }
}

void Bus::ReadData(unsigned int &data)
{
    if (last_ack_master==MASTER_ID_SW){
        wait(read_data_ev_hw);
        data=read_data_hw;
    } else  {
        wait(read_data_ev_sw);
        data=read_data_sw;
    }
    
    BUS_PRINT("ReadData phase");
}

void Bus::WriteData(unsigned int data)
{
    if (current_req.address < MMIO_END) {
        wait(BUS_ACKNOWLEDGE_CYCLES*sc_time(CLOCK_PERIOD_NS,SC_NS));
        hw_minion->mmio_incoming_write_data = data;
        hw_minion->mmio_write_data_ready_ev.notify();
    } else {
        minion_mem->incoming_write_data = data;
        write_data_ev.notify();
    }
    BUS_PRINT("WriteData phase");
}

void Bus::Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len)
{
    wait(request_ev);
    req_addr = current_req.address;
    req_op   = current_req.operation;
    req_len  = current_req.length;
}

void Bus::Acknowledge()
{
    wait(BUS_ACKNOWLEDGE_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    ack_ev.notify();
    ack_done_ev.notify(); 
    BUS_PRINT("Acknowledge sent");
}

void Bus::SendReadData(unsigned int data)
{
    wait(BUS_READ_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    //if(current_req.master_id==MASTER_ID_SW){
    read_data_sw=data;
    read_data_hw=data;
    read_data_ev_sw.notify();
    //} else{
    read_data_ev_hw.notify();
    read_done_ev.notify();
    //}

    BUS_PRINT("SendReadData done");
}

void Bus::ReceiveWriteData(unsigned int &data)
{
    wait(write_data_ev);
    data = shared_write_data;
    wait(BUS_WRITE_CYCLES * sc_time(1, SC_NS));
    BUS_PRINT("ReceiveWriteData done");
}

void Bus::arbiter_thread()
{
    while (true) {

        bool any_pending=false;
        for(int i=0;i<2;i++)
            if(pending[i].active){
                any_pending=true;
                break;
            }
        if(!any_pending)
            wait(request_ev);

        int winner = select_next_master();
        if (winner < 0) continue;

        grant_to_master(winner);
        wait(ack_done_ev);

        if(current_req.operation==BUS_OP_WRITE){
            if(current_req.address<MMIO_END)
                wait(hw_minion->mmio_write_data_ready_ev);
            else
                wait(write_data_ev);
        }
        else
            wait(read_done_ev);;

        current_req.active = false;
        BUS_PRINT("Transaction finished for master " << winner);

        rr_next = (rr_next + 1) % 2;

    }
}

int Bus::select_next_master()
{
    for (int i = 0; i < 2; ++i) {
        int cand = (rr_next + i) % 2;
        if (pending[cand].active) return cand;
    }
    return -1;
}

void Bus::grant_to_master(int mid)
{
    current_req = pending[mid];
    pending[mid].active = false;
    minion_mem->set_current_master(mid);
    BUS_PRINT("Granted to master " << mid);
    if(current_req.address<MMIO_END)
        hw_minion->notify_request(current_req.address,current_req.operation,current_req.length);
    else
        minion_mem->notify_request(current_req.address,current_req.operation,current_req.length);
}

void Bus::data_thread()
{
    while (true) wait(sc_time(1, SC_MS));
}