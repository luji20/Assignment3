#include "bus.h"

Bus::Bus(sc_module_name name)
    : sc_module(name),
      master_export("master_export"),
      minion_export("minion_export")
{
    master_export(*this);
    minion_export(*this); // Bus implements the minion interface

    SC_THREAD(arbiter_thread);

    for (int i = 0; i < 2; ++i) pending[i].active = false;
}

void Bus::Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) {
    wait(BUS_REQUEST_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    if (mst_id >= 2) return;
    if (pending[mst_id].active) return;

    pending[mst_id] = {mst_id, addr, op, len, true};
    BUS_PRINT("Master " << mst_id << " requested @0x" << std::hex << addr << " len=" << len);
    request_ev.notify();
}

bool Bus::WaitForAcknowledge(unsigned int mst_id) {
    while (true) {
        wait(ack_ev);
        // Only proceed if the Arbiter's current request belongs to this master
        if (current_req.master_id == mst_id) {
            BUS_PRINT("Master " << mst_id << " received ACK");
            return true;
        }
    }
}

void Bus::ReadData(unsigned int &data) {
    wait(read_data_ev);
    data = shared_read_data;
    BUS_PRINT("ReadData phase");
}

void Bus::WriteData(unsigned int data) {
    shared_write_data = data;
    // Notify in the NEXT delta cycle to prevent 0-time race conditions
    write_data_ev.notify(SC_ZERO_TIME); 
    BUS_PRINT("WriteData phase");
}

void Bus::Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) {
    wait(minion_request_ev); // Block until Arbiter broadcasts a request
    req_addr = current_req.address;
    req_op   = current_req.operation;
    req_len  = current_req.length;
}

void Bus::Acknowledge() {
    wait(BUS_ACKNOWLEDGE_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    ack_ev.notify();
    BUS_PRINT("Acknowledge sent");
}

void Bus::SendReadData(unsigned int data) {
    wait(BUS_READ_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    shared_read_data = data;
    read_data_ev.notify();
    transaction_done_ev.notify(); // Tell arbiter this word is finished
    BUS_PRINT("SendReadData done");
}

void Bus::ReceiveWriteData(unsigned int &data) {
    wait(write_data_ev); // Wait for master to put data on bus
    data = shared_write_data;
    wait(BUS_WRITE_CYCLES * sc_time(CLOCK_PERIOD_NS, SC_NS));
    transaction_done_ev.notify(); // Tell arbiter this word is finished
    BUS_PRINT("ReceiveWriteData done");
}

void Bus::arbiter_thread() {
    while (true) {
        bool any_pending=false;
        for(int i=0;i<2;i++) {
            if(pending[i].active) {
                any_pending=true; break;
            }
        }
        if(!any_pending) wait(request_ev);

        int winner = select_next_master();
        if (winner < 0) continue;

        grant_to_master(winner);
        
        // Wait for the minion to process all data words in the burst
        for(unsigned int w = 0; w < current_req.length; ++w) {
            wait(transaction_done_ev); 
        }
        BUS_PRINT("Transaction finished for master " << winner);

        rr_next = (rr_next + 1) % 2;
    }
}

int Bus::select_next_master() {
    for (int i = 0; i < 2; ++i) {
        int cand = (rr_next + i) % 2;
        if (pending[cand].active) return cand;
    }
    return -1;
}

void Bus::grant_to_master(int mid) {
    current_req = pending[mid];
    pending[mid].active = false;
    BUS_PRINT("Granted to master " << mid);
    minion_request_ev.notify(); // Wakes up ALL minion threads
}