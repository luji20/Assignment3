# generate_mem.py
start_addr = 0x0400
end_addr = 0xc3900

with open("test_large.mem", "w") as f:
    f.write("# Memory init file - 0x0400 to 0x1000\n")
    f.write("# Format: <hex_byte_address> <hex_32bit_value>\n\n")
    
    value = 1
    # Loop from start_addr to end_addr (inclusive), stepping by 4 bytes
    for addr in range(start_addr, end_addr + 4, 4):
        f.write(f"{addr:08X} {value:08X}\n")
        value += 1

print("Memory file 'test_large.mem' generated successfully.")