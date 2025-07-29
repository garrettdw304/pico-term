#include "vga_graphics.h"
#include "hardware/dma.h"
#include "stdbool.h"
#include "term.h"

// Size of a monospaced character
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

// Terminal rows and columns
#define TERM_ROWS (SCREEN_HEIGHT / CHAR_HEIGHT)
#define TERM_COLS (SCREEN_WIDTH / CHAR_WIDTH)

/// @brief DMA Channel used by term for various things.
static int termChan;
static dma_channel_config shiftUpConfig;
static dma_channel_config clearConfig;

static uint32_t termCol = 0;
static uint32_t termRow = TERM_ROWS - 1;
/// @brief When reaching the end of a row, move to the next automatically?
static bool wrap = true;

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
        termCol = 0;
        termRow++;
        if (termRow >= TERM_ROWS) {
            term_shift_up();
            termRow = TERM_ROWS - 1;
        }
    }
    else if (input == '\n') {

    }
    else if (input == 8) { // backspace
        if (termCol > 0)
            termCol--;
    }
    else if (input < 32) {

    }
    else if (input == 127) { // delete

    }
    else {
        drawChar(termCol * CHAR_WIDTH, termRow * CHAR_HEIGHT, input, WHITE, BLACK, 1);
        advance();
    }
}
