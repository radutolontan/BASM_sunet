#!/usr/bin/env python3
import time
import struct
import construct
import sys
import numpy as np
import serial
import crc16
from octave_lightbar_master.src.RS485Manager import RS485_bus

all_connected_slave_addresses = [0x05, 0x0a]      # List consisting of Slave Board addresses connected to RS485 BUS

def main():
    k_freq = 30 # Hz
    k_sample_rate = 1 / k_freq # Sec
    k_pixel_brightness = 200 # uint8t [0-255]
    main_comms_bus = RS485_bus('/dev/ttyUSB1', all_connected_slave_addresses, strip_brightness = k_pixel_brightness)

    print(f"k_freq: {k_freq}")
    time.sleep(2)

    for slave_cmd in range(0,10000):
        ready_to_send  = 0
        prev_time = time.time()
        while ready_to_send != 1:
                ready_to_send = (time.time() - prev_time) > k_sample_rate
        wait_complete_time = time.time()
        for slave_id in range(4,12):
            # Send message only when ready condition is true!
            main_comms_bus.send_cmd(slave_id,slave_cmd%256)

        print(f"actual_freq: {1/(time.time() - prev_time)}")
        print(f"ratio send/wait: {(time.time()-wait_complete_time)/(wait_complete_time-prev_time)}")


if __name__ == "__main__":
    main()