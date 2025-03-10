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

    // Methods to edit a frame by setting all first no_LEDs to color_to_set
    void edit_bottom_fill(uint32_t color_to_set, uint8_t no_LEDs);

    // Method to create and display a frame for the boot state machine [AWAITING HANDSHAKE]
    void display_boot(); 

    // Method to create and display a frame for the ready state machine [HANDSHAKE COMPLETE]
    void display_ready();

    // Method to create and display a frame for the FFT state machine [HANDSHAKE COMPLETE]
    void display_fft(uint8_t no_LEDs);

    // Public variable prototypes (since they get called from outside the class)
    std::vector<uint32_t> colors_lib;                // [NEOPIXEL] Color Library
    
  private:
    // Private variable prototypes 
    Adafruit_NeoPixel* strip;                        // NeoPixel Strip Pointer
    const uint8_t kdefault_brightness;               // Default LED Brightness

    const uint8_t bar_type;                          // [0 for classic GPIO][1 for NeoPixel Strips]
    const std::vector<uint8_t> GPIO;                 // Vector of GPIO allocations [for NeoPixel, still pass a one-elem. vector for the GPIO pin connected to the Data line]
    const uint8_t output_count;                      // The lightbar has this many LED Pixels / output devices
  
    // Method to import color library
    void import_colors();
};

#endif // LEDCONTROLLER_H