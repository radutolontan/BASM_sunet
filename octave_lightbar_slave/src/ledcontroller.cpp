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
            output_count(LED_count){

    // Initialize the appropriate GPIO usage
    if (bar_type == 1) {
        // For NeoPixel Type
        // Initialize NeoPixel Strip
        strip = new Adafruit_NeoPixel(output_count, GPIO[0], NEO_GRB + NEO_KHZ800);
        strip->begin();            // Initialize the strip
        strip->show();             // Turn off all LEDs initially
        strip->setBrightness(80);  // Adjust brightness (0-255)

        
        // Initialize colors &
        import_colors();
    }
}
    

// import_colors method deffinition
void ledcontroller::import_colors() {
// Initialize Colors
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 0));      // Red
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 0));      // Green
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 255));      // Blue
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 0));    // Yellow
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 255));    // Cyan
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 255));    // Magenta
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 255));  // White
}

void ledcontroller::create_state(uint32_t color_to_set, uint8_t no_LEDs) {
    // Turn off all pixels
    strip->clear();  

    // Turn on the first 5 LEDs (set color to red for visibility)
    for (int i = 0; i < no_LEDs; i++) {
        strip->setPixelColor(i, color_to_set);
    }

    // Apply changes to the strip
    strip->show();  
}

void ledcontroller::display_boot(){
    // Display 10 blue pixels
    create_state(colors_lib[2], 10);
}

void ledcontroller::display_ready(){
    // Display 10 green pixels
    create_state(colors_lib[1], 10);
}



