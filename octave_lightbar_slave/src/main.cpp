#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "RS485_bus.h"
#include "ledcontroller.h"
#include <vector>
#include <algorithm>
   
// ==============================================================
const unsigned char k_slave_addr = 0x05; // === HARDCODED FOR NOW
// ==============================================================

const std::vector<uint8_t>  kcontroller_type = {1, 1, 1, 1};     // ONLY Using Neopixels
const std::vector<uint16_t> kleds_per_strip = {30, 30, 30, 30};  // ONLY Using Neopixels
std::vector<std::vector<uint8_t>> GPIOmap = {
                                              {27}, // Lightbar No.1 has one GPIO for Dout [NeoPixel]
                                              {26}, // Lightbar No.2 has one GPIO for Dout [NeoPixel]  
                                              {25}, // Lightbar No.3 has one GPIO for Dout [NeoPixel]
                                              {33}  // Lightbar No.4 has one GPIO for Dout [NeoPixel]
                                            };

const float k_factor = 30.0f / 256;

// Declare a pointer for the serial bus class object before setup
RS485bus* comms_bus;

// Declare pointers for a vector of ledcontroller instances
std::vector<ledcontroller*> lightbar_vec;

void setup() {
  /*Initialize Lightbars and display boot-state
  */
  for (u_int8_t bar_index = 0; bar_index < 4; bar_index++){
    lightbar_vec.push_back(new ledcontroller(kcontroller_type[bar_index], GPIOmap[bar_index], kleds_per_strip[bar_index]));
  }
  for (ledcontroller* lb : lightbar_vec) { 
        lb->display_boot(); 
  }

  /*Initialize Serial Bus
  This will also wait for a handshake configuration message from the Master [RPi].
  If configured to respond, the ESP32 will send a handshake acknowledge message.*/
  comms_bus = new RS485bus(Serial2, k_slave_addr);

  /*Update brightness and display ready-state
  */
  for (ledcontroller* lb : lightbar_vec) { 
      lb->config_brightness(comms_bus->kparam_brightness);
      lb->display_ready(); 
  }
  // =========================================================
  // ====== FOR DEBUGGING, ALSO INITIALIZE UART0 (via USB)
  // =========================================================
  Serial.begin(115200);
  Serial.println("==========================  INIT  ==============================");

  
}


void loop() {

unsigned long startTime = 0;  // Capture start time
float executionTime = 0.0f;
unsigned long endTime = 0;

  while (true){
    // Read CMD Message from Serial Port
    if (comms_bus->read_frame(comms_bus->kser_cmd_header)){
      // Set first N LEDs to red
      int no_to_turn_on = round(comms_bus->new_frame[4]*k_factor);
      // Call method on each LightBar instance
      for (ledcontroller* lb : lightbar_vec) { // Pointer access
        Serial.println("[SLAVE] - SENDING CMD " + String(no_to_turn_on));
        lb->display_fft(no_to_turn_on); 
      }


      // TIME!
      endTime = micros();  // Capture end time

      // COMPUTE FREQUENCY
      executionTime = (endTime - startTime) / 1000000.0;  // Compute duration
      float freq_display = 1.0f / executionTime;

      // DEBUG
       //Serial.println(executionTime);
      //Serial.println(endTime);
      //Serial.println(startTime);
      //Serial.println("[SLAVE] - DISP_FREQ " + String(freq_display));

      // RESET START TIME
      startTime = micros(); 
    }
  
}
}

void setLEDs(uint8_t cmd){
// FUNCTION DECLARATION
}
