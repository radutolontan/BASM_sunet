#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "RS485_bus.h"
#include "ledcontroller.h"
#include <vector>
#include <algorithm>
   
// ==============================================================
const unsigned char k_slave_addr = 0x05; // === HARDCODED FOR NOW
const bool debug_mode = true;
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

// Declare pointer for the serial bus class object before setup
RS485bus* comms_bus;

// Declare pointer for a vector of ledcontroller objects before setup
std::vector<ledcontroller*> lightbar_vec;

void setup() {  

    // Serial Print for debug ONLY
    if (debug_mode){
        Serial.begin(115200);
    }
  
    // Initialize Lightbars and display boot-state
    for (u_int8_t bar_index = 0; bar_index < 4; bar_index++){ 
        lightbar_vec.push_back(new ledcontroller(kcontroller_type[bar_index], GPIOmap[bar_index], kleds_per_strip[bar_index], debug_mode));
        lightbar_vec[bar_index]->display_boot(); 
        if (debug_mode){
            Serial.println("[ledcontroller] - [SUCCESS] - Init. Strip " + String(bar_index)+ " ; Addr. " + String(k_slave_addr) + " complete!");
        }  
        // POSSIBLE ISSUE!!! - hard-coded no_of_lightbars [4]
    }
    
    /*Initialize Serial Bus
    This will also wait for a handshake configuration message from the Master [RPi].
    The configuration message includes parameters such as the desired led brightness (kparam_brightness)
    If configured to respond, the ESP32 will send a handshake acknowledge message.*/
    comms_bus = new RS485bus(Serial2, k_slave_addr, debug_mode);

    /*Configure lightbars from Params. and display ready-state
    */
    u_int8_t bar_index = 0;
    for (ledcontroller* lb : lightbar_vec) { 
        // Config brightness from run-time param
        lb->config_brightness(comms_bus->kparam_brightness);
        if (debug_mode){
            Serial.println("[ledcontroller] - [INFO] - Brightness Strip " + String(bar_index)+ " set to " + String(comms_bus->kparam_brightness) + "!");
        } 
        // Import colormap if requested
        if (comms_bus->kparam_colormap != 0){
            lb->import_colormap(comms_bus->kparam_colormap, comms_bus->kparam_slavenum, bar_index);
        }
        else{
            if (debug_mode){
            Serial.println("[ledcontroller] - [INFO] - No Colormap Requested - Setting stale CYAN!");
            } 
        }

        // Display ready state
        lb->display_ready(); 
        // Increase index
        bar_index++;
    }
}


void loop() {

    unsigned long startTime = 0;  // Capture start time
    float executionTime = 0.0f;
    unsigned long endTime = 0;

    // Clear display after the first command message is recieved
    bool app_running = false;
    while (!app_running){
    // Read CMD Message from Serial Port
        if (comms_bus->read_frame(comms_bus->kser_cmd_header)){
            uint8_t barindex = 0;
            for (ledcontroller* lb : lightbar_vec) { 
                lb->display_clear();
            }
            app_running = true;
        }
        sleep(0.2);
    }


    while (true){
        // Read CMD Message from Serial Port
        if (comms_bus->read_frame(comms_bus->kser_cmd_header)){
        // Compose command vector for all lightbars
        std::vector<int> no_to_turn_on;
        for (u_int8_t bar_index = 0; bar_index < 4; bar_index++){
            no_to_turn_on.push_back(round(comms_bus->new_frame[4+bar_index]*k_factor));
        }
        

        // Change all lightbar commands using command vector
        u_int8_t bar_index = 0;
        for (ledcontroller* lb : lightbar_vec) { 
            lb->display_fft(no_to_turn_on[bar_index]); 
            bar_index++;
        }


        // TIME!
        endTime = micros();  // Capture end time

        // COMPUTE FREQUENCY
        executionTime = (endTime - startTime) / 1000000.0;  // Compute duration
        float freq_display = 1.0f / executionTime;


        // RESET START TIME
        startTime = micros(); 
        }
  
}
}

void setLEDs(uint8_t cmd){
// FUNCTION DECLARATION
}



/* =====LITTLEFS MEMORY FORMAT CODE=====*/

// #include "FS.h"
// #include "LittleFS.h"

// void setup() {
//     Serial.begin(115200);
    
//     // Try mounting LittleFS first
//     if (!LittleFS.begin(true)) { // 'true' enables auto-format if mount fails
//         Serial.println("LittleFS mount failed");
//         return;
//     }

//     Serial.println("Formatting LittleFS...");
//     if (LittleFS.format()) {
//         Serial.println("LittleFS formatted successfully");
//     } else {
//         Serial.println("LittleFS format failed");
//     }
// }

// void loop() {
// }

/* =====LITTLEFS MEMORY FORMAT CODE=====*/

/* =====LITTLEFS MEMORY CONFIRM CODE=====*/

// #include "FS.h"
// #include "LittleFS.h"

// void listFiles() {
//     Serial.println("Listing files in LittleFS...");

//     File root = LittleFS.open("/");
//     if (!root) {
//         Serial.println("Failed to open root directory");
//         return;
//     }

//     File file = root.openNextFile();
//     while (file) {
//         Serial.print("File: ");
//         Serial.print(file.name());
//         Serial.print(" - Size: ");
//         Serial.print(file.size());
//         Serial.println(" bytes");
        
//         file = root.openNextFile();
//     }
// }

// void setup() {
//     Serial.begin(115200);
    
//     if (!LittleFS.begin(true)) { // Mount LittleFS (true = format if failed)
//         Serial.println("LittleFS mount failed");
//         return;
//     }

//     listFiles();  // Call function to list files
// }

// void loop() {
// }
/* =====LITTLEFS MEMORY CONFIRM CODE=====*/