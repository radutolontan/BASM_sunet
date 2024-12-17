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
all_connected_slave_addresses = [0x00]      # List consisting of Slave Board addresses connected to RS485 BUS
slave_hs_ack_byte = 0xCA
slave_config_params = [1]                   # Configuration Parameters for Slave Boards [CURRENTLY ONLY A DUMMY]


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
        # If the message frame is of type bytes, turn it into integers for summation
        if all(isinstance(item, bytes) for item in message):
            self.check_sum_in = [ord(byte) for byte in message]
        else:
            self.check_sum_in = message
        # Compute checksum value using a modulo 256 [such that it fits in a byte]
        return sum(self.check_sum_in)%256

    def __slave_handshake(self):
        # CONFIRM BILATERAL COMMUNICATIONS TO ALL SLAVE (MONITOR) BOARDS [ESP32s]
        #self.monitor_hs_confirmed = np.zeros(len(slave_addresses))

        # [TODO] [DONE] IMPLEMENT DYNAMIC PAYLOAD LENGTH BASED ON len(slave_config_params)
        # [TODO] IMPLEMENT CHECKING FOR ALL MONITOR/SLAVE BOARDS within slave_addresses
        # [TODO] IMPLEMENT PARSING FOR HANDSHAKE ACKNOWLEDGE MESSAGES

        # Send the handshake config message at 10 Hz until the handshake ack message is recieved
        #self.all_handshakes_complete = 0
        for addr in all_connected_slave_addresses:
            # Send Configuration Handshake Message to Slave #i
            self.__handshake_config(slave_address = addr)
            # Process Acknowledge Handshake Message from Slave #i
            self.read_frame(header_bytes = k_ser_hs_header, slave_address = addr)
            # Confirm the ACK Bytes were recieved
            if all(ord(self.new_frame[byte]) == slave_hs_ack_byte for byte in range(4,6)):
                print("[SERIAL] - Established Comms. w. Slave Addr." + str(addr))
            time.sleep(0.02)


        print("[SERIAL] - ALL COMMS ACTIVE!")

    def __handshake_config(self, slave_address):
        '''
        SEND A CONFIGURATION MESSAGE TO THE SLAVE ESP32 BOARDS. This message is also used as a handshake 
        to confirm TX on the RPI and RX on the ESP32
        '''
        # Compute checksum for outgoing configuration message
        checksum_in = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, payload_length = len(slave_config_params) + 5, config_param_1 = 0x01, check_sum=0x00))
        checksum_out = self.__checksum_compute(checksum_in)
        # Pack the raw message
        self.config_msg = handshake_config_msg.build(dict(header=k_ser_hs_header, slave_id=slave_address, payload_length = len(slave_config_params) + 5, config_param_1 = 0x01, check_sum=checksum_out))
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
        else:
            print("[SERIAL] - Timed-out waiting for Msg. from Slave Addr." + str(slave_address))


    def __find_sync(self, header_bytes, slave_address):
        '''
        Method used to find a sync sequence. Sync sequence is defined as the two header bytes, followed by the expected slave_address.
        The method will run continously until a sync (header+address) is found OR a timeout is experienced.
        It returns 1 when the sync sequence is found.
        It returns 0 for a timeout.
        '''
        t_init = time.time()
        while True:
            self.new_frame = [self.ser.read(size=1)]
            if ord(self.new_frame[-1]) == header_bytes[0]:          # Check for first header byte
                self.new_frame.append(self.ser.read(size=1))
                if ord(self.new_frame[-1]) == header_bytes[1]:      # Check for second header byte
                    self.new_frame.append(self.ser.read(size=1))
                    if ord(self.new_frame[-1]) == slave_address:    # Check for the slave address
                        return 1
            
            # TIMEOUT NOT IMPLEMENTED CORRECTLY
            # Check for timeout
            print(time.time() - t_init)
            if (time.time() - t_init > 5):
                return 0

    def __read_data(self):
        '''
        Method used to read 
        1. frame_size (UINT_8) - includes header (2 bytes), slave address (1 byte), frame_sz (1 byte), data bytes (n), checksum (1 byte)
        2. data_bytes (UINT_8) - as many as n = (frame_size - 5)
        3. checksum (UINT_8) - a modul0 256 checksum value
        '''
        # Read the total frame size
        self.new_frame.append(self.ser.read(size=1))
        frame_size = ord(self.new_frame[-1])

        # Read the n data bytes, and the checksum
        for i in range (0, frame_size - 4):
            self.new_frame.append(self.ser.read(size=1))

        return 1



    def close_comms(self):
        if self.ser.is_open:
            self.ser.close()


    
