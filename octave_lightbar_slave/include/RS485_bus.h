// RS485Manager.h

/*  RS485Manager.h
    This header file is home to the class and methods prototypes that support serial communications
    between the lightbar Master [RPi] and Slave [ESP32].
*/

#ifndef RS485_BUS_H
#define RS485_BUS_H

#include <stdint.h> 
#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>

class RS485bus {
  public:
    // Constructor
    RS485bus(HardwareSerial& serial_num, unsigned char slave_address);
    
    // Method to read a frame (used for both handshake and command messages)
    bool read_frame(const std::vector<unsigned char>& header);

    // Method to command a Master Handshake (Configuration Message (and if selected) Acknowledge Message)
    void master_handshake();

    // Public variable prototypes (since they get called from outside the class)
    const std::vector<unsigned char> kser_cmd_header;// Header Bytes for Command Message Frame
    const unsigned char kslave_addr;                 // Slave Address
    std::vector<unsigned char> new_frame;            // Byte vector for serial frame being read
    std::vector<unsigned char> old_frame;            // Byte vector for previous serial frame

  private:
    // Provate variable prototypes 
    HardwareSerial& serialPort;                      // Serial Port
    const uint8_t kser_rx_pin, kser_tx_pin;          // UART GPIO Pins
    const uint8_t kser_RTS_pin;                      // RTS GPIO Pin
    const int kser_baud;                             // Serial Communications Baudrate
    const std::vector<unsigned char> kser_hs_header; // Header Bytes for Handshake Message Frame

    // Method to compute frame checksum
    int checksum_compute(const std::vector<unsigned char>& frame);

    // Method to find a sync sequence at the start of a frame
    bool find_sync(const std::vector<unsigned char>& header);

    // Method to find a sync sequence at the start of a frame
    bool read_data();

    // Method to send an acknowledgement to the Handshake
    void send_acknowledge();

};

#endif // RS485_BUS_H