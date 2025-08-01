#include "vga_graphics.h"
#include "hardware/dma.h"
#include "stdbool.h"
#include "term.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

// Size of a monospaced character
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

// Terminal rows and columns
#define TERM_ROWS (SCREEN_HEIGHT / CHAR_HEIGHT)
#define TERM_COLS (SCREEN_WIDTH / CHAR_WIDTH)

typedef struct {
    /// @brief One pixel per byte for simplicity.
    uint8_t pixels[CHAR_WIDTH * CHAR_HEIGHT];
} TermChar;

/// @brief DMA Channel used by term for various things.
static int termChan;
static dma_channel_config shiftUpConfig;
static dma_channel_config clearConfig;

static uint32_t termCol = 0;
static uint32_t termRow = TERM_ROWS - 1;
/// @brief When reaching the end of a row, move to the next automatically?
static bool wrap = true;

// ===================================== Cursor ==================================
static TermChar charAtCursor = {0};
/// @brief BLACK = transparent, other than that color does not matter.
static TermChar cursorTexture = {0};

static char colorAt(int x, int y) {
    if (x >= SCREEN_WIDTH) return 0;
    if (y >= SCREEN_HEIGHT) return 0;

    char colors = vga_data_array[((y * SCREEN_WIDTH) + x) / 2];
    return ((x & 1) == 0 ? (colors) : (colors >> 3)) & 0b00000111;
}

static char getColor(TermChar *c, int x, int y) {
    if (x >= CHAR_WIDTH) return 0;
    if (y >= CHAR_HEIGHT) return 0;

    return c->pixels[(y * CHAR_WIDTH) + x];
}

static void setColor(TermChar *c, char color, int x, int y) {
    if (x >= SCREEN_WIDTH) return;
    if (y >= SCREEN_HEIGHT) return;

    c->pixels[(y * CHAR_WIDTH) + x] = color;
}

/// @brief Removes the cursor at termCol,termRow by drawing charAtCursor at the same location.
static void eraseCursor(void) {
    for (int y = 0; y < CHAR_HEIGHT; y++)
        for (int x = 0; x < CHAR_WIDTH; x++)
            drawPixel(termCol * CHAR_WIDTH + x, termRow * CHAR_HEIGHT + y, getColor(&charAtCursor, x, y));
}

/// @brief Draws the cursor at termCol,termRow.
static void drawCursor(void) {
    // Save character at cursor
    for (int y = 0; y < CHAR_HEIGHT; y++)
        for (int x = 0; x < CHAR_WIDTH; x++)
            setColor(&charAtCursor, colorAt(termCol * CHAR_WIDTH + x, termRow * CHAR_HEIGHT + y), x, y);

    // Draw cursor
    for (int y = 0; y < CHAR_HEIGHT; y++)
        for (int x = 0; x < CHAR_WIDTH; x++)
        {
            if (getColor(&cursorTexture, x, y) == BLACK) // BLACK = transparent
                continue;
            int xPos = termCol * CHAR_WIDTH + x;
            int yPos = termRow * CHAR_HEIGHT + y;
            drawPixel(xPos, yPos, colorAt(xPos, yPos) == WHITE ? BLACK : WHITE);
        }
}
// ===================================== Cursor END ==============================

/// @brief Advances the cursor.
static void advance(void) {
    termCol++;
    if (termCol >= TERM_COLS) {
        if (wrap) {
            termCol = 0;
            termRow++;
            if (termRow >= TERM_ROWS) {
                term_shift_up();
                termRow = TERM_ROWS - 1;
            }
        }
        else
            termCol--;
    }
}

void term_init(void) {
    termChan = dma_claim_unused_channel(true);

    shiftUpConfig = dma_channel_get_default_config(termChan);
    channel_config_set_transfer_data_size(&shiftUpConfig, DMA_SIZE_8);
    channel_config_set_read_increment(&shiftUpConfig, true);
    channel_config_set_write_increment(&shiftUpConfig, true);

    clearConfig = dma_channel_get_default_config(termChan);
    channel_config_set_transfer_data_size(&clearConfig, DMA_SIZE_8);
    channel_config_set_read_increment(&clearConfig, false);
    channel_config_set_write_increment(&clearConfig, true);

    // Horizontal bar cursor
    for (int i = 0; i < CHAR_WIDTH; i++) {
        setColor(&cursorTexture, WHITE, i, 6);
        setColor(&cursorTexture, WHITE, i, 7);
    }

    // Vertical bar cursor
    // for (int i = 0; i < CHAR_HEIGHT; i++) {
    //     setColor(&cursorTexture, WHITE, 0, i);
    //     setColor(&cursorTexture, WHITE, 1, i);
    // }

    // Full cursor
    // for (int i = 0; i < count_of(cursorTexture.pixels); i++)
    //     cursorTexture.pixels[i] = WHITE;

    drawCursor();
}

/// @brief Move all rows up one.
void term_shift_up(void) {
    uint8_t *from = vga_data_array + ROW_BYTES;
    uint8_t *to = vga_data_array;
    uint32_t txfers = TXCOUNT - ROW_BYTES;

    // Shift rows upwards
    dma_channel_configure(termChan, &shiftUpConfig, to, from, txfers, true);
    dma_channel_wait_for_finish_blocking(termChan);

    // Clear bottom row
    term_clear_row(TERM_ROWS - 1);
}

/// @brief Clears an entire row.
/// @param rowIndex The row to clear.
void term_clear_row(uint32_t row) {
    if (row >= TERM_ROWS) return;

    uint8_t clearColor = BLACK;

    uint8_t *from = &clearColor;
    uint8_t *to = vga_data_array + (row * ROW_BYTES);
    uint32_t txfers = ROW_BYTES;

    dma_channel_configure(termChan, &clearConfig, to, from, txfers, true);
    dma_channel_wait_for_finish_blocking(termChan);
}

/// @brief Clears from the start of row, to and including col.
void term_clear_from_start(uint32_t row, uint32_t col) {
    if (row >= TERM_ROWS) return;
    if (col >= TERM_COLS) return;

    if (col == TERM_COLS - 1)
        term_clear_row(row);
    else
        fillRect(0, row * CHAR_HEIGHT, (col + 1) * CHAR_WIDTH, CHAR_HEIGHT, BLACK); // TODO: DMA?
}

/// @brief Clears from the end of row, to and including col.
void term_clear_from_end(uint32_t row, uint32_t col) {
    if (row >= TERM_ROWS) return;
    if (col >= TERM_COLS) return;

    if (col == 0)
        term_clear_row(row);
    else
        fillRect(col * CHAR_WIDTH, row * CHAR_HEIGHT, SCREEN_WIDTH - col * CHAR_WIDTH, CHAR_HEIGHT, BLACK); // TODO: DMA?
}

void term_process(char input) {
    if (input == '\033') { // ESC

    }
    else if (input == '\r') {
        eraseCursor();
        termCol = 0;
        termRow++;
        if (termRow >= TERM_ROWS) {
            term_shift_up();
            termRow = TERM_ROWS - 1;
        }
        drawCursor();
    }
    else if (input == '\n') {

    }
    else if (input == 8) { // backspace
        if (termCol > 0)
        {
            eraseCursor();
            termCol--;
            drawCursor();
        }
    }
    else if (input < 32) { // control characters

    }
    else if (input == 127) { // delete

    }
    else { // printable characters
        drawChar(termCol * CHAR_WIDTH, termRow * CHAR_HEIGHT, input, WHITE, BLACK, 1);
        advance();
        drawCursor();
    }
}
