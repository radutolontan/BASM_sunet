// ledcontroller.h

/*  ledcontroller.h
    This header file is home to the class and methods prototypes that support control of ESP32 GPIOs, either for direct control [HI/LO],
    or through an Adafruit NeoPixel LED strip
*/

#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <stdint.h> 
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <vector>

class ledcontroller {
  public:
    // Constructor
    ledcontroller(uint8_t output_type, std::vector<uint8_t> GPIO_handle, uint16_t LED_count);

    // Methods to display a desired state state
    void create_state(uint32_t color_to_set, uint8_t no_LEDs);

    void display_boot(); 

    void display_ready();

    // Public variable prototypes (since they get called from outside the class)
    const uint8_t bar_type;                          // [0 for classic GPIO][1 for NeoPixel Strips]
    const std::vector<uint8_t> GPIO;                 // Vector of GPIO allocations [for NeoPixel, still pass a one-elem. vector for the GPIO pin connected to the Data line]
    const uint8_t output_count;                      // The lightbar has this many LED / output devices
    std::vector<uint32_t> colors_lib;                // [NEOPIXEL] Color Library
    
  private:
    // Private variable prototypes 
    Adafruit_NeoPixel* strip;                        // NeoPixel Strip Pointer

    // Method to import color library
    void import_colors();
};

#endif // LEDCONTROLLER_H