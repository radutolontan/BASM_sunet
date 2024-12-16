#!/usr/bin/env python3
from construct import Struct, Int8ul, Int16ul, Int16ub, Array, Computed
import serial
import time
import numpy as np
import crc16

single_slave_msg = Struct(
    "header" / Array(2, Int8ul), # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "cmd" / Int16ul,             # 16-bit unsigned integer for the commanded value
    "check_sum" / Int8ul,        # 8-bit unsigned integer for the modulo 256 checksum
)

handshake_config_msg = Struct(
    "header" / Array(2, Int8ul), # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "payload_length" / Int8ul,   # 8-bit unsigned integer for the total number of bytes in packet (incl. header, slave address, payload length, checksum)
    "config_param_1" / Int8ul,   # 8-bit unsigned integer for "CONFIG PARAM 1"
    "check_sum" / Int8ul,        # 8-bit unsigned integer for the modulo 256 checksum
)

k_ser_cmd_header = [0xDE, 0xAD] # Two-byte header for COMMAND MESSAGES
k_ser_hs_header  = [0xFF, 0xFE] # Two-byte header for HANDSHAKE MESSAGES
slave_addresses = [0x00]      # List consisting of Slave Board addresses connected to RS485 BUS
slave_config_params = [1]            # Configuration Parameters for Slave Boards [CURRENTLY ONLY A DUMMY]


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
        
        if not self.ser.is_open:
            # Open serial port if not already available
            self.ser.open()

        # Confirm communications from slave boards are active
        self.__slave_handshake()
            
    def send_cmd(self, slave_address, slave_command):
        # Compute checksum for message
        checksum_in = single_slave_msg.build(dict(header=k_ser_cmd_header, slave_id=slave_address, cmd=slave_command, check_sum=0x00))
        checksum_out = self.__checksum_compute(checksum_in)
        # Pack message w. checksum
        self.cmd_msg = single_slave_msg.build(dict(header=k_ser_cmd_header, slave_id=slave_address, cmd=slave_command, check_sum= checksum_out))
        # Send the message
        try:
            # Send the message over the serial port
            self.ser.write(self.cmd_msg)
            print(f"Message sent: {self.cmd_msg.hex()}")
        except serial.SerialException as e:
            print(f"Error: {e}")

    def __checksum_compute(self, message):
        # Pack the message including a zeroed-out checksum
        self.check_sum_in = message
        # Compute checksum value using a modulo 256 [such that it fits in a byte]
        return sum(self.check_sum_in)%256

    def __slave_handshake(self):
        # CONFIRM BILATERAL COMMUNICATIONS TO ALL SLAVE (MONITOR) BOARDS [ESP32s]
        self.monitor_hs_confirmed = np.zeros(len(slave_addresses))

        # [TODO] IMPLEMENT DYNAMIC PAYLOAD LENGTH BASED ON len(slave_config_params)
        # [TODO] IMPLEMENT CHECKING FOR ALL MONITOR/SLAVE BOARDS within slave_addresses
        # [TODO] IMPLEMENT PARSING FOR HANDSHAKE ACKNOWLEDGE MESSAGES

        # Send the handshake config message at 10 Hz until the handshake ack message is recieved
        self.all_handshakes_complete = 0
        while (self.all_handshakes_complete != 1):
            # Send message to Slave 0
            self.__handshake_config(slave_address = slave_addresses[0])
            time.sleep(0.1)

        print("[RS485] - Comms Active!")

    def __handshake_config(self, slave_address):
        # SEND A CONFIGURATION MESSAGE TO THE SLAVE ESP32 BOARDS. This message is also used as a handshake 
        # to confirm TX on the RPI and RX on the ESP32

        # Compute checksum for outgoing configuration message
        checksum_in = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, payload_length = 0x06, config_param_1 = 0x01, check_sum=0x00))
        checksum_out = self.__checksum_compute(checksum_in)
        # Pack the raw message
        self.config_msg = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, payload_length = 0x06, config_param_1 = 0x01, check_sum=checksum_out))
        try:
            # Send the message over the serial port
            self.ser.write(self.config_msg)
            print(f"Configuration Message Sent: {self.config_msg.hex()}")
        except serial.SerialException as e:
            print(f"Error: {e}")

    def close_comms(self):
        if self.ser.is_open:
            self.ser.close()


    
