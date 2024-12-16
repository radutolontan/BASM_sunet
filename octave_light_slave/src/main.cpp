#include <Arduino.h>
#include <HardwareSerial.h>

// Create a HardwareSerial object for UART0
HardwareSerial mySerial(2);

// SLAVE ADDRESS CURRENTLY HARDCODED! WILL BE SELECTED USING DIPSWITCHES
const uint8_t k_slave_addr = 0;

// Serial frame headers for handhake and command messages
const uint8_t k_ser_hs_header[]  = {0xFF, 0xFE}; // HEADER FOR HANDSHAKE MESSAGES
const uint8_t k_ser_cmd_header[] = {0xDE, 0xAD}; // HEADER FOR COMMAND MESSAGES


// put your setup code here, to run once:
const uint8_t LED_GPIO_Pins[] = {15,2,0,4};
const u_int8_t n_LED_GPIO_Pins = sizeof(LED_GPIO_Pins) / sizeof(LED_GPIO_Pins[0]);
const uint8_t RS485_RTS_Pin = 18;


const float k_factor = 1.0f;

// Function Prototypes
void setLEDs(int cmd);
bool serial_hs_config();
void serial_hs_ack();
uint8_t calculateChecksum(uint8_t *data, uint8_t length);

// DEBUG FUNCTION PROTOTYPES
void debugSerial(uint8_t byte);
void debugMessage(uint8_t string);


void setup() {
  // Initialize all GPIO LED CTRL Pins
  for (int i = 0; i < n_LED_GPIO_Pins; i++) {
      pinMode(LED_GPIO_Pins[i], OUTPUT);
    }

  // Initialize UART2 at a baud rate of 115200, w. RTS pin LOW
  mySerial.begin(115200, SERIAL_8N1, 16, 17);  // RX = Pin 16, TX = Pin 17
  pinMode(RS485_RTS_Pin, OUTPUT);
  digitalWrite(RS485_RTS_Pin, LOW);
  bool rec_conf_msg = 0;

  // FOR DEBUGGING USE UART0 
  Serial.begin(115200); // Initialize default serial for debugging (via USB)

  // Read handshake - configuration message from RPi
  while (rec_conf_msg != 1){
    rec_conf_msg = serial_hs_config();
    delay(10);
  }

  Serial.println("[SLAVE ADDR. " + String(k_slave_addr) + "] - Recieved Config. Msg. from Master!");
  digitalWrite(RS485_RTS_Pin, HIGH);
}

void loop() {
  // Turn ON All LEDs
  // for (int i = 0; i < 20; i++) {
  //   // Access the value of I from the array
  //   delay(1000);
  //   setLEDs(i);    
  //   delay(1000);
  //   }
  // Define a hex value (for example, 0xAB)
  uint8_t hexValue = 0xAB;  // This packs 0xAB into a single byte
  
  // Send the packed byte via UART1
  mySerial.write(hexValue);  // Send the byte over UART1

  delay(50);
  

}

bool serial_hs_config(){
  uint8_t serial_packet[256];
  if (mySerial.available()) {
      // Step 1: Wait for the first header byte
      uint8_t header_byte_1 = mySerial.read();
      debugSerial(header_byte_1);
      Serial.println("HEADER1 OK!");
      if (header_byte_1 == k_ser_hs_header[0]) {
        // RECIEVED FIRST HEADER BYTE!
        serial_packet[0] = header_byte_1;

        if (mySerial.available()) {
          // Step 2: Wait for the second header byte
          uint8_t header_byte_2 = mySerial.read();
          debugSerial(header_byte_2);
          Serial.println("HEADER2 OK!");
          if (header_byte_2 == k_ser_hs_header[1]) {
            // RECIEVED SECOND HEADER BYTE!
            serial_packet[1] = header_byte_2;

            if (mySerial.available()) {
              // Step 3: Wait for the correct slave address
              uint8_t slave_addr = mySerial.read();
              debugSerial(slave_addr);
              Serial.println("SLAVE ADDR OK!");
              if (slave_addr == k_slave_addr) {
                // RECIEVED CORRECT SLAVE ADDRESS!
                serial_packet[2] = slave_addr;

                if (mySerial.available()) {
                  // Step 4: Read the data length byte
                  uint8_t payloadLength = mySerial.read();
                  debugSerial(payloadLength);
                  Serial.println("READ PKG LENGTH!");
                  // RECIEVED NO. OF PAYLOAD BYTES!
                  serial_packet[3] = payloadLength;

                  // Step 5: Read the data bytes into a buffer
                  int bytesRead = 4;
                  while (bytesRead < (payloadLength - 1) && mySerial.available()) {
                    serial_packet[bytesRead++] = mySerial.read();
                    debugSerial(serial_packet[bytesRead-1]);
                    Serial.println("READ PAYLOAD BYTE!");
                  }
                  // RECIEVED PAYLOAD BYTES

                  if (mySerial.available()) {
                    // Step 6: Read and verify checksum byte
                    uint8_t receivedChecksum = mySerial.read();
                    debugSerial(receivedChecksum);
                    Serial.println("READ CHECKSUM BYTE!");
                    uint8_t calculatedChecksum = calculateChecksum(serial_packet, bytesRead);

                      if (receivedChecksum == calculatedChecksum) {
                      // CHECKSUM VERIFIED
                        serial_packet[bytesRead++] = receivedChecksum;
                        Serial.println("CONFIGURATION PACKET RECIEVED!");
                        return 1;
                      }
                  }
                }
              }
            }
          }
        }
      }
  }
  // If at any point a step is incomplete, return 0!
  return 0;
}

void serial_hs_ack(){
  
}

uint8_t calculateChecksum(uint8_t *data, uint8_t length) {
  uint32_t sum = 0;
  for (int i = 0; i < length; i++) {
    sum += data[i];
  }
  uint8_t checksum = sum % 256;
  Serial.println("Computed Checksum - ");
  Serial.print(checksum);
  return checksum;
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

void debugSerial(uint8_t byte){
  Serial.print("Received byte: ");
  Serial.println(byte, HEX);
}
