#include <Arduino.h>
#include <HardwareSerial.h>
#include "RS485_bus.h"
#include <vector>
   
// =========================================================
const unsigned char k_slave_addr = 0x10; // === HARDCODED
// =========================================================


// put your setup code here, to run once:
const uint8_t LED_GPIO_Pins[] = {15,2,0,4};
const u_int8_t n_LED_GPIO_Pins = sizeof(LED_GPIO_Pins) / sizeof(LED_GPIO_Pins[0]);

const float k_factor = 1.0f;

// Function Prototypes
void setLEDs(int cmd);
//bool serial_hs_config();
//void serial_hs_ack();
//uint8_t calculateChecksum(uint8_t *data, uint8_t length);

// DEBUG FUNCTION PROTOTYPES
//void debugSerial(uint8_t byte);
//void debugMessage(uint8_t string);


void setup() {
RS485bus comms_bus(Serial2, k_slave_addr);

}

void loop() {
  
}

void setLEDs(int cmd){

  int no_to_turn_on = floor(cmd/k_factor);
  // Turn on the appropriate no. of LEDs
  for (int i = 0; i < no_to_turn_on; i++) {
    // Access the value of I from the array
      digitalWrite(LED_GPIO_Pins[i], HIGH);
    }
  // Turn off the appropriate no. of LEDs
  for (int i = no_to_turn_on; i < n_LED_GPIO_Pins; i++) {
    // Access the value of I from the array
      digitalWrite(LED_GPIO_Pins[i], LOW);
    }
}

// void debugSerial(uint8_t byte){
//   Serial.print("Received byte: ");
//   Serial.println(byte, HEX);
// }
