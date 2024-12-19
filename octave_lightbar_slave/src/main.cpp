#include <Arduino.h>
#include <HardwareSerial.h>
#include "RS485_bus.h"
#include <vector>
   
// ==============================================================
const unsigned char k_slave_addr = 0x0a; // === HARDCODED FOR NOW
// ==============================================================


// put your setup code here, to run once:
const uint8_t LED_GPIO_Pins[] = {15,2,0,4,19};
const u_int8_t n_LED_GPIO_Pins = sizeof(LED_GPIO_Pins) / sizeof(LED_GPIO_Pins[0]);

const float k_factor = 0.0234375f;

// Declare a pointer for the serial bus class object before setup
RS485bus* comms_bus;

// Function Prototypes
void setLEDs(uint8_t cmd);

void setup() {
  /*Initialize Serial Bus
  This will also wait for a handshake configuration message from the Master [RPi].
  If configured to respond, the ESP32 will send a handshake acknowledge message.*/
  comms_bus = new RS485bus(Serial2, k_slave_addr);


  /*Initialize LEDs
  */
    // Initialize all GPIO LED CTRL Pins
  for (int i = 0; i < n_LED_GPIO_Pins; i++) {
      pinMode(LED_GPIO_Pins[i], OUTPUT);
    }

  // =========================================================
  // ====== FOR DEBUGGING, ALSO INITIALIZE UART0 (via USB)
  // =========================================================
  Serial.begin(115200);
  Serial.println("==========================  APPLICATION ==============================");

}

void loop() {
  // Read CMD Message from Serial Port
  if (comms_bus->read_frame(comms_bus->kser_cmd_header)){
    // We recieved a command message!
     // Serial.println("[SLAVE] - CMD " + String(comms_bus->new_frame[4]));
     setLEDs(comms_bus->new_frame[4]);
  }
}

void setLEDs(uint8_t cmd){

  int no_to_turn_on = round(cmd*k_factor);
  Serial.print(no_to_turn_on);
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
