// ledcontroller.cpp
    
/*  ledcontroller.cpp
    This header file is home to the class and methods declarations that support control of ESP32 GPIOs, either for direct control [HI/LO],
    or through an Adafruit NeoPixel LED strip
*/

#include "ledcontroller.h"
#include <stdint.h> 
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <vector>
#include <cstdint>
#include <FS.h>
#include <LittleFS.h>

// Constructor definition
ledcontroller::ledcontroller(uint8_t output_type, std::vector<uint8_t> GPIO_handle, uint16_t LED_count, bool debugmode):
            bar_type(output_type),
            GPIO(GPIO_handle),
            output_count(LED_count),
            last_cmd(0),
            kdefault_brightness(80),
            debug_mode(debugmode){ 

    // Initialize the appropriate GPIO usage
    if (bar_type == 1) {
        // Initialize NeoPixel Strip
        strip = new Adafruit_NeoPixel(output_count, GPIO[0], NEO_GRB + NEO_KHZ800);
        strip->begin();                             // Initialize the strip
        strip->show();                              // Turn off all LEDs initially
        // Initialize default brightness. It will be updated once config. param is recieved
        strip->setBrightness(kdefault_brightness);  // Set default brightness
        // Initialize color library and colormap files
        import_colorlib();
        colormap_files_vec = {"/calibration.csv", "/sunset.csv", "/coastline.csv", "/apples.csv", "/balloon.csv", "/dark.csv"};
        colormap_matrix.clear();

    }
}
    
// import_colorslib method deffinition
void ledcontroller::import_colorlib() {
// Initialize Color library
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 0));        // 0 OFF 
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 0));      // 1 Red
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 0));      // 2 Green
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 255));      // 3 Blue
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 165, 0));    // 4 Orange
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 0));    // 5 Yellow
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 255));    // 6 Cyan
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 255));    // 7 Magenta
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 255));  // 8 White
}

// Method to import a specific colormap for the LED matrix
void ledcontroller::import_colormap(uint8_t conf_colormapid, uint8_t conf_slaveno, uint8_t lightbar_no){
    // If colormap is imported for the first time, read the csv file
    if (colormap_matrix.empty()){
        if (debug_mode){
            Serial.println("[ledcontroller] - [INFO]- Importing colormap id " + String(conf_colormapid) + 
                                                        "on Slave No. " + String(conf_slaveno) + 
                                                        "on Lightbar No. " + String(lightbar_no) + " ...");
        }  
        
        // Open the appropriate colormap CSV file
        if (!LittleFS.begin(true)) {
            if (debug_mode){
                Serial.println("[ledcontroller] - [ERROR] - LittleFS mount failed!");
            }  
            return;
        }   

        // [conf_colormapid-1] adjustment accounts for 0 which is no_colormap
        File file = LittleFS.open(colormap_files_vec[conf_colormapid-1], "r");
        if (!file) {
            if (debug_mode){
                Serial.println("[ledcontroller] - [ERROR] - Colormap not found!");
            }  
            display_error();
            return;
        }

        char line[512]; 

        // Read Lines
        while (file.available()) {
            int len = file.readBytesUntil('\n', line, sizeof(line) - 1);
            line[len] = '\0';  // Null-terminate the string

            std::vector<uint32_t> row;  // Store row data
            char *token = strtok(line, ",");  // Split by commas

            while (token) {
                row.push_back(strtoul(token, NULL, 10));  // Convert to uint32_t
                token = strtok(NULL, ",");
            }

            if (!row.empty()) {
                colormap_matrix.push_back(row);
            }

        }

        file.close(); 
        
        }    

    if (debug_mode){
                Serial.print("[ledcontroller] - [INFO] - colormap vector: ");
    }  
    // Grab specific column in the colormap_matrix for [conf_slaveno - lightbar_no combination]
    colormap_bar_vec.clear();
    for (int row = 0; row < colormap_matrix.size(); row++) {
        colormap_bar_vec.push_back(colormap_matrix[row][4*conf_slaveno+lightbar_no]); 
        // POSSIBLE ISSUE!!! - 4*conf_slaveno+lightbar_no assumes 4 lightbars per slave
        if (debug_mode){
                Serial.print(colormap_bar_vec[row] + String(" "));
    }  
    }

    if (debug_mode){
        Serial.println();
        Serial.println("[ledcontroller] - [SUCCESS] - Colormap imported. LittleFS mount closed");
    } 
    
}

// Method to display colormap metadata to confirm correct load
void ledcontroller::metadata_colormap(){
    Serial.println("====== COLORMAP METADATA ======");
    // MATRIX DATA
    Serial.printf("Matrix Size: %d x %d\n", colormap_matrix.size(), colormap_matrix[0].size());

    for (size_t row = 0; row < colormap_matrix.size(); row++) {
        for (size_t col = 0; col < colormap_matrix[row].size(); col++) {
            Serial.printf("%08X ", colormap_matrix[row][col]);  // Print as Hex
        }
        Serial.println();
    }
}

// Method to update the brightness of an led strip
void ledcontroller::config_brightness(uint8_t conf_brightness) {
    if (bar_type == 1) { // FOR NEOPIXEL
        // Set the brightness of the entire NeoPixel strip to conf_brightness
        strip->setBrightness(conf_brightness);
    }
}

// Method to create and display a frame for the FFT [RUNNING STATE MACHINE]
void ledcontroller::display_fft(uint8_t new_cmd){
    if (bar_type == 1) { // FOR NEOPIXEL
        /*As opposed to the display_<state> methods, display_FFT doesn't start with a clean slate each time
        Rather, to save time, it turns on or off LEDs compared to the last_cmd to achieve the new_cmd
        */
        if (new_cmd > last_cmd){ // TURN ON LEDs!
            for (int i = last_cmd; i < new_cmd; i++) {  
                if (colormap_matrix.empty()){
                    // STALE COLOR OPERATION
                    strip->setPixelColor(i, colors_lib[6]);
                }
                else{
                    // OPERATION WITH COLORMAP
                    strip->setPixelColor(i, colormap_bar_vec[i]);
                }
            }
        }
        else { // TURN OFF LEDs!
            for (int i = new_cmd; i < last_cmd; i++) { 
                strip->setPixelColor(i, colors_lib[0]); 
            }

        }

        // Copy the new command into last_cmd
        last_cmd = new_cmd;

        // Display changes on the strip
        strip->show();  
    }
}

// Method to create and display a frame for the boot state machine [AWAITING HANDSHAKE]
void ledcontroller::display_boot(){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Build a 10 blue pixel tower
        edit_bottom_fill(colors_lib[3], 10); // colors_lib[3] - BLUE

        // Display changes on the strip
        strip->show();  
    }
}

// Method to create and display a frame for the ready state machine [HANDSHAKE + CONFIG COMPLETE]
void ledcontroller::display_ready(){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Build a 10 green pixel tower
        edit_bottom_fill(colors_lib[2], 10); // colors_lib[2] - GREEN

        // Display changes on the strip
        strip->show();
    }  
}

// Method to create and display a frame for the fault state machine [FAULT!]
void ledcontroller::display_error(){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Build a 10 green pixel tower
        edit_bottom_fill(colors_lib[1], 10); // colors_lib[1] - RED

        // Display changes on the strip
        strip->show();
    }  
}

// Method to clear the whole light_bar display
void ledcontroller::display_clear(){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Display changes on the strip
        strip->show();
    }  
}

// Methods used by 'display_[state]' methods to set a solid color fill for the bottom n LEDs on a strip
void ledcontroller::edit_bottom_fill(uint32_t color_to_set, uint8_t no_LEDs) {
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn on the first no_LEDs and set them to color_to_set
        for (int i = 0; i < no_LEDs; i++) { 
            strip->setPixelColor(i, color_to_set);
        }
    }
}

