#!/usr/bin/env python3
import time
import struct
import construct
import sys
import numpy as np
import serial
import crc16
from src.RS485Manager import RS485_bus

def main():
    main_comms_bus = RS485_bus('/dev/ttyUSB0')
    slave_id = 8
    slave_cmd = 14
    main_comms_bus.send_cmd(slave_id,slave_cmd)
    main_comms_bus.close_comms()

if __name__ == "__main__":
    main()