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

// Constructor definition
ledcontroller::ledcontroller(uint8_t output_type, std::vector<uint8_t> GPIO_handle, uint16_t LED_count):
            bar_type(output_type),
            GPIO(GPIO_handle),
            output_count(LED_count),
            kdefault_brightness(80){ 

    // Initialize the appropriate GPIO usage
    if (bar_type == 1) {
        // For NeoPixel Type
        // Initialize NeoPixel Strip
        strip = new Adafruit_NeoPixel(output_count, GPIO[0], NEO_GRB + NEO_KHZ800);
        strip->begin();                             // Initialize the strip
        strip->show();                              // Turn off all LEDs initially
        // Initialize default brightness. It will be updated once config. param is recieved
        strip->setBrightness(kdefault_brightness);  // Adjust brightness (0-255)
        
        // Initialize color library and colormap
        import_colors();
    }
}
    
// import_colors method deffinition
void ledcontroller::import_colors() {
// Initialize Colors
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 0));      // Red
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 0));      // Green
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 255));      // Blue
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 165, 0));    // Orange
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 0));    // Yellow
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 255));    // Cyan
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 255));    // Magenta
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 255));  // White
}

// Methods to update the brightness of an led strip
void ledcontroller::config_brightness(uint8_t conf_brightness) {
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn on the first no_LEDs and set them to color_to_set
        strip->setBrightness(conf_brightness);
    }
}


// Methods to edit a frame by setting all first no_LEDs to color_to_set
void ledcontroller::edit_bottom_fill(uint32_t color_to_set, uint8_t no_LEDs) {
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn on the first no_LEDs and set them to color_to_set
        for (int i = 0; i < no_LEDs; i++) { 
            strip->setPixelColor(i, color_to_set);
        }
    }
}

// Method to create and display a frame for the boot state machine [AWAITING HANDSHAKE]
void ledcontroller::display_fft(uint8_t no_LEDs){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Build a no_LEDs Orange pixel tower
        edit_bottom_fill(colors_lib[3], no_LEDs); // colors_lib[3] - ORANGE

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
        edit_bottom_fill(colors_lib[2], 10); // colors_lib[2] - BLUE

        // Display changes on the strip
        strip->show();  
    }
}

// Method to create and display a frame for the ready state machine [HANDSHAKE COMPLETE]
void ledcontroller::display_ready(){
    if (bar_type == 1) { // FOR NEOPIXEL
        // Turn off all pixels
        strip->clear();  

        // Build a 10 green pixel tower
        edit_bottom_fill(colors_lib[1], 10); // colors_lib[1] - GREEN

        // Display changes on the strip
        strip->show();
    }  
}



