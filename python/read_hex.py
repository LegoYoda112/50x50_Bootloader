import numpy as np
import math

def int_to_hexstr(data, digits):
    return f'{data:0{digits}x}'

# Open file
file = open("50x50_USB2CAN.hex", "r")

# ELAR = file.readline()[1:-1]
# print(ELAR)

# data_line = f.readline()[1:-1]
# data_line = f.readline()[1:-1]
# data_line = f.readline()[1:-1]

full_data = []
full_data_address = 0

memory_address = 0x0000

MEM_START_OFFSET = 0x08008000

for data_line in file:
    data_line = data_line[1:-1]
    data_length = data_line[0:2]
    data_length_int = int(data_length, 16)

    address = data_line[2:6]
    int_address = int(address, 16)
    record_type = data_line[6:8]

    if(record_type == "04"):
        address_second_half = int(data_line[8:12], 16)

    memory_address = (int_address & 0xFFFFF) | address_second_half << 8*2

    if(record_type == "00"):
        offset_address = memory_address - MEM_START_OFFSET
        
        while(full_data_address != offset_address):
            input()
            print(full_data_address, offset_address)

            full_data += ['0', '0'] # Add null byte
            full_data_address += 1 # Add byte

        print(int_to_hexstr(memory_address, 8))
        data = data_line[8:8 + 2 * data_length_int]
        full_data += data

        full_data_address += data_length_int

        # print(data_line)
        # print(data_length, address, record_type, data)

        # print(len(data))

        # data_list_int = list(bytearray.fromhex(data))
        # full_message_int = list(bytearray.fromhex(data_line))
        # checksum = sum(full_message_int[:-1]) & 0xFF
        # print(256 - checksum)
        # print(checksum_int)

file.close()


full_data_split = []
counter = 0
new_row = []
for byte in full_data:
    new_row.append(byte)
    counter += 1
    if(counter == 16 * 2):
        counter = 0
        full_data_split.append(new_row)
        new_row = []

out_file = ""
address_value = 0
for row in full_data_split:
    out_line = ":"

    row = np.array(row)

    data_line = "".join(row)


    # DATA LENGTH
    out_line += int_to_hexstr(int(len(data_line)/2), 2) + ""

    # ADDRESS 
    out_line += int_to_hexstr(address_value, 4) + ""

    # RECORD TYPE
    out_line += "00"

    # DATA
    out_line += data_line

    # Checksum
    # TODO: Actually make this work
    out_line += "FF"

    # print(out_line)
    out_file += out_line + "\r"
    address_value += 16


f = open("16_bit_aligned.hex", "w")
f.write(out_file)
f.close()