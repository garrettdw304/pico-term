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
 * NOTE
 *  - This is a translation of the display primitives
 *    for the PIC32 written by Bruce Land and students
 *
 */


// Give the I/O pins that we're using some names that make sense - usable in main()
enum vga_pins {HSYNC=16, VSYNC, RED_PIN, GREEN_PIN, BLUE_PIN};

// We can only produce 8 (3-bit) colors, so let's give them readable names - usable in main()
enum colors {BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE};

// (internal) Screen width/height.
// These can be adjusted lower than 640x480 as long as they are adjusted equally and by a multiple of 2.
// If these are adjusted, the rgb.pio timings must also be adjusted proportionally.
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define INTERNAL_TO_EXTERNAL_SCALE (640 / SCREEN_WIDTH)

/// @brief The number of pixel bytes in a single line.
#define LINE_BYTES (SCREEN_WIDTH / 2)

/// @brief The number of pixel bytes in a single terminal row.
#define ROW_BYTES (LINE_BYTES * 8)

// VGA timing constants
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE (LINE_BYTES - 1)    // number of bytes sent per line - 1

// Length of the pixel array
#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT / 2) // Total internal pixels/2 (since we have 2 pixels per byte)

// Pixel color array that is DMA's to the PIO machines and
// a pointer to the ADDRESS of this color array.
// Note that this array is automatically initialized to all 0's (black)
extern unsigned char vga_data_array[];

// VGA primitives - usable in main
void initVGA(void);
void drawPixel(short x, short y, char color);
void drawVLine(short x, short y, short h, char color);
void drawHLine(short x, short y, short w, char color);
void drawLine(short x0, short y0, short x1, short y1, char color);
void drawRect(short x, short y, short w, short h, char color);
void drawCircle(short x0, short y0, short r, char color);
void drawCircleHelper( short x0, short y0, short r, unsigned char cornername, char color);
void fillCircle(short x0, short y0, short r, char color);
void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color);
void drawRoundRect(short x, short y, short w, short h, short r, char color);
void fillRoundRect(short x, short y, short w, short h, short r, char color);
void fillRect(short x, short y, short w, short h, char color);
void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size);
void setCursor(short x, short y);
void setTextColor(char c);
void setTextColor2(char c, char bg);
void setTextSize(unsigned char s);
void setTextWrap(char w);
void tft_write(unsigned char c);
void writeString(char* str);
