#!/usr/bin/env python3
from construct import Struct, Int8ul, Int16ul, Int16ub, Array, Computed
import serial
import time
import numpy as np
import crc16

single_slave_msg = Struct(
    "header" / Array(2, Int8ul), # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "frame_length" / Int8ul,     # 8-bit unsigned integer for the total number of bytes in packet (incl. header, slave address, payload length, checksum)
    "cmd_1" / Int8ul,            # 8-bit unsigned integer for the commanded value [bar 1]
    "cmd_2" / Int8ul,            # 8-bit unsigned integer for the commanded value [bar 2]
    "cmd_3" / Int8ul,            # 8-bit unsigned integer for the commanded value [bar 3]
    "cmd_4" / Int8ul,            # 8-bit unsigned integer for the commanded value [bar 4]
    "check_sum" / Int8ul,        # 8-bit unsigned integer for the modulo 256 checksum
)

handshake_config_msg = Struct(
    "header" / Array(2, Int8ul), # Two-byte header
    "slave_id" / Int8ul,         # 8-bit unsigned integer for the reciever slave address
    "frame_length" / Int8ul,     # 8-bit unsigned integer for the total number of bytes in packet (incl. header, slave address, payload length, checksum)
    "config_param_1" / Int8ul,   # 8-bit unsigned integer for the DEFAULT LED BRIGHTNESS - "CONFIG PARAM 1"
    "check_sum" / Int8ul,        # 8-bit unsigned integer for the modulo 256 checksum
)

k_ser_cmd_header = [0xDE, 0xAD] # Two-byte header for COMMAND MESSAGES
k_ser_hs_header  = [0xFF, 0xFE] # Two-byte header for HANDSHAKE MESSAGES
k_ser_timeout = 2              # Timeout in seconds for recieving serial messages
slave_hs_ack_byte = 0xCA


class RS485_bus():
    def __init__(self, port_ID, connected_slaves, slave_config_params):
        self.port_ID = port_ID
        self.bitrate = 115200

        # Declare and open serial port communications
        self.ser = serial.Serial(
                                port=self.port_ID,
                                baudrate=self.bitrate,
                                parity = serial.PARITY_NONE,
                                bytesize=serial.EIGHTBITS,
                                stopbits=serial.STOPBITS_ONE, 
                                timeout=k_ser_timeout,   
                                )
        
        if not self.ser.is_open:
            # Open serial port if not already available
            self.ser.open()

        # Confirm communications from slave boards are active
        for addr in connected_slaves:
            self.__slave_handshake(slave_addr = addr, config_params = slave_config_params)

        print("[SERIAL] - ALL COMMS ACTIVE !")

    def send_cmd(self, slave_address, slave_command):
        # Compute checksum for message
        checksum_in = single_slave_msg.build(dict(header=k_ser_cmd_header, slave_id=slave_address, frame_length=len(slave_command) + 5, cmd_1=slave_command[0], cmd_2=slave_command[1], cmd_3=slave_command[2], cmd_4=slave_command[3],check_sum=0x00))
        checksum_out = self.__checksum_compute(checksum_in)
        # Pack message w. checksum
        self.cmd_msg = single_slave_msg.build(dict(header=k_ser_cmd_header, slave_id=slave_address, frame_length=len(slave_command) + 5, cmd_1=slave_command[0], cmd_2=slave_command[1], cmd_3=slave_command[2], cmd_4=slave_command[3], check_sum= checksum_out))
        # Send the message
        try:
            # Send the message over the serial port
            self.ser.write(self.cmd_msg)
            print(f"Message sent: {self.cmd_msg.hex()}")
        except serial.SerialException as e:
            print(f"Error: {e}")

    def __slave_handshake(self, slave_addr, config_params):
        '''
        CONFIRM BILATERAL COMMUNICATION TO SLAVE (MONITOR) BOARDS [ESP32s]
        '''        
        print("[SERIAL] - Initiating Handshake w. Slave Addr." + str(slave_addr) + " ...")
        handshake_confirmed = 0
        # Configure the read_frame method to recieve Handshake messages
        while not handshake_confirmed:
            # Send Configuration Handshake Message to Slave #slave_address
            self.__send_config(slave_address = slave_addr, config_vec = config_params)
                # config_1 is the Default LED Brightness
            # Try to read an ACK frame
            if (self.read_frame(header_bytes = k_ser_hs_header, slave_address = slave_addr)):
                # Confirm the ACK Bytes were recieved
                if all(ord(self.new_frame[byte]) == slave_hs_ack_byte for byte in range(4,6)):
                    handshake_confirmed = 1
                    print("[SERIAL] - Handshake OK w. Slave Addr." + str(slave_addr) + " !")

    def __send_config(self, slave_address, config_vec):
        '''
        SEND A CONFIGURATION MESSAGE TO THE SLAVE ESP32 BOARD. This message is also used as a handshake 
        to confirm TX on the RPI and RX on the ESP32
        '''
        # Compute checksum for outgoing configuration message
        checksum_in = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, frame_length = len(config_vec) + 5, config_param_1 = config_vec[0], check_sum=0x00))
        checksum_out = self.__checksum_compute(checksum_in)
        # Pack the raw message
        self.config_msg = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, frame_length = len(config_vec) + 5, config_param_1 = config_vec[0], check_sum=checksum_out))
        try:
            # Send the message over the serial port
            self.ser.write(self.config_msg)
        except serial.SerialException as e:
            print(f"Error: {e}")

    def read_frame(self, header_bytes, slave_address):
        '''
        header_bytes identifies the type of message being read
            k_ser_cmd_header for a COMMAND MESSAGE
            k_ser_hs_header  for a CONFIG. MESSAGE
        '''
        # Allocate storage for the new frame
        self.new_frame = []
        
        if self.__find_sync(header_bytes, slave_address):  # If a sync sequence is found
            if self.__read_data():                         # If the required number of bytes is read
                # Confirm checksum
                checksum_compute = self.__checksum_compute(self.new_frame[:-1])
                if checksum_compute == ord(self.new_frame[-1]): # If the checksum is correct
                    self.old_frame = self.new_frame
                    return 1
                else:
                    print("[SERIAL] - Recieved Corrupted Msg. from Slave Addr." + str(slave_address))
                    print("Retrying ...")
                    return 0
        else:
            print("[SERIAL] - Timed-out waiting for Msg. from Slave Addr." + str(slave_address))
            print("Retrying ...")
            return 0

    def __find_sync(self, header_bytes, slave_address):
        '''
        Method used to find a sync sequence. Sync sequence is defined as the two header bytes, followed by the expected slave_address.
        The method will run continously until a sync (header+address) is found OR a timeout is experienced.
        It returns 1 when the sync sequence is found.
        It returns 0 for a timeout.
        '''
        t_init = time.time()
        while True:
            # Try to read a new byte using the timeout defined in the __init__ method
            new_byte = self.ser.read(size=1)

            # Check if a byte was read
            if new_byte:
                if len(self.new_frame) == 0 and ord(new_byte) == header_bytes[0]:  # Check for first header byte
                    self.new_frame = [new_byte]
                elif len(self.new_frame) == 1 and ord(new_byte) == header_bytes[1]:# Check for second header byte
                    self.new_frame.append(new_byte)    
                elif len(self.new_frame) == 2:                                     # Check if both header bytes were recieved
                    if ord(new_byte) == slave_address:                             # Correct slave address
                        self.new_frame.append(new_byte)
                        return 1
                    else:                                                          # Correct slave address
                        return 0

            # Check for timeout
            elif (time.time() - t_init > k_ser_timeout):
                return 0

    def __read_data(self):
        '''
        Method used to read 
        1. frame_size (UINT_8) - includes header (2 bytes), slave address (1 byte), frame_sz (1 byte), data bytes (n), checksum (1 byte)
        2. data_bytes (UINT_8) - as many as n = (frame_size - 5)
        3. checksum (UINT_8) - a modul0 256 checksum value
        '''
        t_init = time.time()
        while True:
            # Try to read a new byte using the timeout defined in the __init__ method
            new_byte = self.ser.read(size=1)

            # Check if a byte was read
            if new_byte:
                # Append new byte to data frame
                self.new_frame.append(new_byte)
                if len(self.new_frame) == 4:                                       # Check for Frame Length byte
                    frame_size = ord(self.new_frame[-1])
                elif len(self.new_frame) == frame_size:
                    return 1
            
            # Check for timeout
            elif (time.time() - t_init > k_ser_timeout):
                return 0

    def __checksum_compute(self, message):
        # If the message frame is of type bytes, turn it into integers for summation
        if all(isinstance(item, bytes) for item in message):
            self.check_sum_in = [ord(byte) for byte in message]
        else:
            self.check_sum_in = message
        # Compute checksum value using a modulo 256 [such that it fits in a byte]
        return sum(self.check_sum_in)%256

    def close_comms(self):
        if self.ser.is_open:
            self.ser.close()


    
