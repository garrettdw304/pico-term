/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 330 ohm resistor ---> VGA Red
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 */

// VGA graphics library
#include "vga_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize the VGA screen
    initVGA() ;

    // Draw some filled rectangles
    fillRect(0, 0, 176, 50, BLUE); // blue box
    fillRect(0, 50, 176, 50, RED); // red box
    fillRect(0, 100, 176, 50, GREEN); // green box

    // Write some text
    setTextColor(WHITE);
    setCursor(50, 50);
    setTextSize(1);
    writeString("HELLO WORLD!");

    while (true) tight_loop_contents();
}
