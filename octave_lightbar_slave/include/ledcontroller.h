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
#include <FS.h>
#include <LittleFS.h> 

class ledcontroller {
  public:
    // Constructor
    ledcontroller(uint8_t output_type, std::vector<uint8_t> GPIO_handle, uint16_t LED_count, bool debugmode);

    // Methods used by 'display_[state]' methods to set a solid color fill for the bottom n LEDs on a strip
    void edit_bottom_fill(uint32_t color_to_set, uint8_t no_LEDs);

    // Method to create and display a frame for the boot state machine [AWAITING HANDSHAKE]
    void display_boot(); 

    // Method to create and display a frame for the ready state machine [HANDSHAKE + CONFIG COMPLETE]
    void display_ready();

    // Method to create and display a frame for the fault state machine [FAULT!]
    void display_error();

    // Method to clear the whole light_bar display
    void display_clear();

    // Method to create and display a frame for the FFT [RUNNING STATE MACHINE]
    void display_fft(uint8_t new_cmd);

    // Method to update the brightness of an led strip
    void config_brightness(uint8_t conf_brightness);

    // Method to import a specific colormap for the LED matrix
    void import_colormap(uint8_t conf_colormapid, uint8_t conf_slaveno, uint8_t lightbar_no);

    // Public variable prototypes (since they get called from outside the class)
    std::vector<uint32_t> colors_lib;                // [NEOPIXEL] Color Library
    
  private:
    // Private variable prototypes 
    Adafruit_NeoPixel* strip;                        // NeoPixel Strip Pointer
    const uint8_t kdefault_brightness;               // Default LED Brightness on Boot-up
    const uint8_t bar_type;                          // [0 for classic GPIO][1 for NeoPixel Strips]
    const std::vector<uint8_t> GPIO;                 // Vector of GPIO allocations [for NeoPixel, still pass a one-elem. vector for the GPIO pin connected to the Data line]
    const uint8_t output_count;                      // The lightbar has this many LED Pixels / output devices
    std::vector<String> colormap_files_vec;          // Vector with colormap filenames
    std::vector<std::vector<uint32_t>> colormap_matrix;  // Matrix with entire colormap loaded
    std::vector<uint32_t> colormap_bar_vec;          // Vector with colormap_matrix entries for specific lightbar instance
    uint8_t last_cmd;                                // Last command to the lightbar
    const bool debug_mode;                           // In debug mode TRUE, Serial Debug Messages are enabled
    
    // Method to import color library [used for display_state and basic fft displays]
    void import_colorlib();

    // Method to display metadata for imported colormap
    void metadata_colormap();
    

};

#endif // LEDCONTROLLER_H