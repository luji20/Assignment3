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
        bus_minion_port->Listen(addr, op, len); // Calls bus via port

        // Ignore requests meant for main memory
        if (addr >= MMIO_END) {
            continue; 
        }

        bus_minion_port->Acknowledge();
        
        if (op == BUS_OP_WRITE && len == 1)
        {
            unsigned int data = 0;
            bus_minion_port->ReceiveWriteData(data);

            switch (addr)
            {
                case MMIO_HW_CTRL:
                    ctrl = data;
                    if (ctrl & HW_CTRL_RESET){
                        status=0;
                    }
                    if (ctrl & HW_CTRL_START) {
                        start_event.notify();
                        status |= HW_STATUS_BUSY;
                        status &= ~HW_STATUS_DONE;
                    }
                    break;
                case MMIO_HW_ADDR_A:  addr_a = data; break;
                case MMIO_HW_ADDR_B:  addr_b = data; break;
                case MMIO_HW_ADDR_C:  addr_c = data; break;
                case MMIO_HW_SIZE:    size   = data; break;
                case MMIO_HW_I:       i      = data; break;
                case MMIO_HW_J:       j      = data; break;
            }
        }
        else if (op == BUS_OP_READ && len == 1) {
            unsigned int val = (addr == MMIO_HW_STATUS) ? status : 0;
            bus_minion_port->SendReadData(val);
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

        // Clear done flag at start of each computation
        status &= ~HW_STATUS_DONE;
        status |= HW_STATUS_BUSY;

        std::cout << "[HW] Starting k-loop offload for i=" << i << " j=" << j << std::endl;

        // 1. BURST READ: Fetch the entire row of Matrix A
        // addr_a points to A[n][i][0], which is the start of a contiguous row
        std::vector<unsigned int> row_a(size, 0); 
        
        bus_master_port->Request(MASTER_ID_HW, addr_a, BUS_OP_READ, size);
        bus_master_port->WaitForAcknowledge(MASTER_ID_HW);
        
        for (unsigned int k = 0; k < size; ++k) {
            bus_master_port->ReadData(row_a[k]);
        }
        std::cout << "[HW] Burst read of Matrix A row complete.\n";

        // 2. COMPUTE: Single reads for Matrix B and accumulate
        int acc = 0;
        for (unsigned int k = 0; k < size; ++k)
        {
            // Calculate strided address for column of B
            unsigned int b_addr = addr_b + k * size * WORD_SIZE_BYTES;
            
            unsigned int a_val = row_a[k]; // Taken from fast local burst buffer
            unsigned int b_val = bus_read_word(b_addr); // Single bus transaction
            
            acc += (int)a_val * (int)b_val;
        }

        // 3. WRITE: Store the final accumulated result
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
    bus_master_port->Request(MASTER_ID_HW, addr, BUS_OP_READ, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_HW);
    unsigned int data = 0;
    bus_master_port->ReadData(data);
    return data;
}

void hw_component::bus_write_word(unsigned int addr, unsigned int data)
{
    bus_master_port->Request(MASTER_ID_HW, addr, BUS_OP_WRITE, 1);
    bus_master_port->WaitForAcknowledge(MASTER_ID_HW);
    bus_master_port->WriteData(data);
}
