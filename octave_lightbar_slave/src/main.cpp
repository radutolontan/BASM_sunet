#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "RS485_bus.h"
#include "ledcontroller.h"
#include <vector>
#include <algorithm>
   
// ==============================================================
const unsigned char k_slave_addr = 0x0a; // === HARDCODED FOR NOW
// ==============================================================


const float k_factor = 60.0f / 256;

// Declare a pointer for the serial bus class object before setup
RS485bus* comms_bus;

// Declare pointers for the ledcontroller objects
ledcontroller* lightbar1;
ledcontroller* lightbar2;

// Function Prototypes
//void setLEDs(uint8_t cmd);

void setup() {
  /*Initialize Lightbars and display boot-state
  */
  lightbar1 = new ledcontroller(1, {25}, 60);
  lightbar2 = new ledcontroller(1, {33}, 60);
  lightbar1->display_boot();
  lightbar2->display_boot();

  /*Initialize Serial Bus
  This will also wait for a handshake configuration message from the Master [RPi].
  If configured to respond, the ESP32 will send a handshake acknowledge message.*/
  comms_bus = new RS485bus(Serial2, k_slave_addr);
  lightbar1->display_ready();
  lightbar2->display_ready();


  // =========================================================
  // ====== FOR DEBUGGING, ALSO INITIALIZE UART0 (via USB)
  // =========================================================
  Serial.begin(115200);
  Serial.println("==========================  APPLICATION ==============================");

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
      lightbar1->create_state(lightbar1->colors_lib[0], no_to_turn_on);
      lightbar2->create_state(lightbar2->colors_lib[0], no_to_turn_on);


      // TIME!
      endTime = micros();  // Capture end time

      // COMPUTE FREQUENCY
      executionTime = (endTime - startTime) / 1000000.0;  // Compute duration
      float freq_display = 1.0f / executionTime;

      // DEBUG
      Serial.println(executionTime);
      Serial.println(endTime);
      Serial.println(startTime);
      Serial.println("[SLAVE] - DISP_FREQ " + String(freq_display));

      // RESET START TIME
      startTime = micros(); 
    }
  
}
}

void setLEDs(uint8_t cmd){
// FUNCTION DECLARATION
}
