#!/usr/bin/env python3
from construct import Struct, Int8ul, Int16ul, Int16ub, Array, Computed
import serial
import crc16

single_slave_msg = Struct(
    "header" / Array(2, Int8ul),           # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "cmd" / Int16ul,             # 16-bit unsigned integer for the commanded value
    "crc" / Int16ul,             # 16-bit unsigned integer for the crc
)

single_slave_msg_no_crc = Struct(
    "header" / Array(2, Int8ul),           # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "cmd" / Int16ul,             # 16-bit unsigned integer for the commanded value
)

k_ser_header = [0xDE, 0xAD] # Two-byte header

class RS485_bus():
    def __init__(self, port_ID):
        self.port_ID = port_ID
        self.bitrate = 115200

        # Declare and open serial port communications
        self.ser = serial.Serial(
                                port=self.port_ID,
                                baudrate=self.bitrate,
                                parity = serial.PARITY_NONE,
                                bytesize=serial.EIGHTBITS,
                                stopbits=serial.STOPBITS_ONE,    
                                )
        # Confirm communications from slave boards are active
        self.__confirm_comms()
        
        if not self.ser.is_open:
            # Open serial port if not already available
            self.ser.open()
            
    def send_cmd(self, slave_address, slave_command):
        self.slave_addr = slave_address
        self.slave_cmd  = slave_command
        # Compute crc for message
        self.__crc_compute()
        # Pack the raw message
        self.raw_msg = single_slave_msg.build(dict(header=k_ser_header, slave_id=self.slave_addr, cmd=self.slave_cmd, crc=self.crc_out))
        # Send the message
        try:
            # Send the message over the serial port
            self.ser.write(self.raw_msg)
            print(f"Message sent: {self.raw_msg.hex()}")
        except serial.SerialException as e:
            print(f"Error: {e}")

    def __crc_compute(self):
        # Pack the message excluding the yet-to-be-calculated CRC16 value
        self.crc_in = single_slave_msg_no_crc.build(dict(header=k_ser_header, slave_id=self.slave_addr, cmd=self.slave_cmd))
        # Compute CRC value using the CRC-16-CCITT algorithm
        print(f"Message sent to crc: {self.crc_in.hex()}")
        self.crc_out = crc16.crc16xmodem(self.crc_in)

    def __confirm_comms(self):
        # To be implemented!
        print("[RS485] - Comms Active!")

    def close_comms(self):
        if self.ser.is_open:
            self.ser.close()


    
