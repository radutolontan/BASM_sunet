// RS485bus.cpp

/*  RS485Manager.cpp
    This source file is home to the class and methods declarations that support serial communications
    between the lightbar Master [RPi] and Slave [ESP32].
*/

#include "RS485_bus.h"
#include <stdint.h> 
#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#define TIMEOUT 2000

// Constructor definition
RS485bus::RS485bus(HardwareSerial& serial_num, unsigned char slave_address):
            serialPort(serial_num),
            kslave_addr(slave_address),
            kser_baud(115200),
            kser_rx_pin(16),
            kser_tx_pin(17),
            kser_RTS_pin(18),
            kser_hs_header({0xFF, 0xFE}),
            kser_cmd_header({0xDE, 0xAD}){

    // Initialize UART2 using the TX/RX Pins
    serialPort.begin(kser_baud, SERIAL_8N1, kser_rx_pin, kser_tx_pin);
    // Initialize RTS GPIO to LOW
    pinMode(kser_RTS_pin, OUTPUT);
    digitalWrite(kser_RTS_pin, LOW);

    // =========================================================
    // ====== FOR DEBUGGING, ALSO INITIALIZE UART0 (via USB)
    // =========================================================
    Serial.begin(115200);
    Serial.println("==========================  HANDSHAKE ==============================");

    // Complete the configuration and handshake with the master
    master_handshake();

    // =========================================================
    // ====== FOR DEBUGGING, ALSO INITIALIZE UART0 (via USB)
    // =========================================================
    if (Serial) {
        Serial.println("==========================  HANDSHAKE ==============================");
        Serial.end();
    }
}
    
// read_frame method definition
bool RS485bus::read_frame(const std::vector<unsigned char>& header) {
    // Clear the values of the last frame
    new_frame.clear();

    // Check if a sync sequence is found
    if (find_sync(header) == true){
        // Check if the data can be read
        if (read_data() == true){
            // Confirm checksum by ignoring the last byte in the new_frame
            std::vector<unsigned char> checksum_in(new_frame.begin(), new_frame.end() - 1);
            unsigned char checksum_out = checksum_compute(checksum_in);
            if (checksum_out == new_frame.back()){
                //Serial.println("[READ_FRAME] - Checksum CORRECT! ");
                old_frame = new_frame;
                return true;
            }
            else{return false;} // INCORRECT CHECKSUM!
            
        }
    }
    // Serial.println("[READ_FRAME] - ERROR!");
    return false; // For any error, return false
}

// checksum_compute method definition
int RS485bus::checksum_compute(const std::vector<unsigned char>& frame) {
    int sum = 0;
    //Serial.println("[CHECKSUM_COMPUTE] - ENTERED");
    //Serial.println("[CHECKSUM_COMPUTE] - FRAME ");
    for (unsigned char byte : frame) {
        sum += byte;
    }
    // Cast the result of the chechsum back into byte format
    unsigned char check_sum= static_cast<unsigned char>(sum%256);
    //Serial.println("[CHECKSUM_COMPUTE] - CHECKSUM_OUT  ");
    //Serial.print(check_sum);
    //Serial.println();
    return check_sum;
}

// find_sync method definition
bool RS485bus::find_sync(const std::vector<unsigned char>& header) {

    //Serial.println("[FIND_SYNC] - ENTERED");

    unsigned long t_init = millis();
  
    while (millis() - t_init < TIMEOUT) {
        if (serialPort.available()){
            // Try to read one byte
            unsigned char new_byte = serialPort.read();
            //Serial.println("[FIND_SYNC] - RECIEVED BYTE " + String(new_byte));

            // If a byte was in fact recieved
            if (new_byte != -1) {
                // Check for the sync sequence bytes
                if (new_frame.size() == 0 && new_byte == header[0]){new_frame.push_back(new_byte);}      // Check for header byte 1
                else if (new_frame.size() == 1 && new_byte == header[1]){new_frame.push_back(new_byte);} // Check for header byte 2
                    else if (new_frame.size() == 2){        // If we found the header bytes
                        if (new_byte == kslave_addr){       // Correct slave address
                            new_frame.push_back(new_byte);
                            // Serial.println("[FIND_SYNC] - ADDRESS OK");
                            return true;}
                        else {                              // Incorrect slave address  
                            // Serial.println("[FIND_SYNC] - INCORRECT ADDRESS");
                            return false;}                 
                    }
            } 
        }
    }
    // If a sync sequence was not recieved for TIMEOUT milliseconds, a timeout occured
    //Serial.println("[FIND_SYNC] - TIMED_OUT!");
    return false;
}

// read_data method definition
bool RS485bus::read_data() {
    
    // After confirming the sync sequence, this method reads the packet size, data_bytes, and check_sum
    unsigned long t_init = millis();
    int frame_size;

    //Serial.println("[READ_DATA] - ENTERED");
  
    while (millis() - t_init < TIMEOUT) {
        // Try to read one byte
        unsigned char new_byte = serialPort.read();
        //Serial.println("[READ_DATA] - RECIEVED BYTE " + String(new_byte));

        // If a byte was in fact recieved
        if (new_byte != -1) {
            // Append new byte to message frame
            new_frame.push_back(new_byte);
            if (new_frame.size() == 4){frame_size = static_cast<int>(new_frame[3]);} // Store Frame Length
            else if (new_frame.size() == frame_size){                                // Check if all bytes have been recieved
                //Serial.println("[READ_DATA] - FINISHED");
                return true;
                }                         
        }
    }

    // If a full message was not recieved for TIMEOUT milliseconds, a timeout occured
    return false;
}

// master_handshake method definition
void RS485bus::master_handshake() {
    // Recieve Configuration / handshake message from Master (RPi) and respond with a handshake acknowledge message
    bool configuration_recieved = false;

    // Continously read messages until the configuration handshake message to the expected address is recieved 
    while (configuration_recieved == false){
        if (read_frame(kser_hs_header)){
            // STORE THE CONFIGURATION PARAMETERS
            kparam_brightness = new_frame[4];
            configuration_recieved = true;
        }
        else{
            delay(20);
            Serial.println("[MASTER_HANDSHAKE] - RETRY!");
        }
    }

    // IF ACKNOWLEDGE HANDSHAKE MESSAGE IS ENABLED (PENDING ALOCATION OF GPIO RTS PIN)
    send_acknowledge();
}

// send_ack method definition
void RS485bus::send_acknowledge() {
    // Set RTS pin HIGH to enable ESP32 to Write to the RS485 Bus
    digitalWrite(kser_RTS_pin, HIGH);

    // Create the ack frame with the checksum field set to 0
    std::vector<unsigned char> ack_frame = {kser_hs_header[0], kser_hs_header[1], kslave_addr, 0x07, 0xCA, 0xCA};

    // Compute outgoing message checksum and repack frame
    unsigned char checksum_out = checksum_compute(ack_frame);
    ack_frame.push_back(checksum_out);

    // Send the 7-byte frame
    serialPort.write(ack_frame.data(), 7);  

    // Allow 0.2 sec. to elapse before setting the RTS pin LOW again
    delay(200);
    digitalWrite(kser_RTS_pin, LOW);
}
 