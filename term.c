#include "vga_graphics.h"
#include "hardware/dma.h"
#include "stdbool.h"

// Terminal rows and columns
#define TERM_ROWS 30
#define TERM_COLS 40

/// @brief DMA Channel used by term for various things.
int termChan;
dma_channel_config shiftUpConfig;
dma_channel_config clearConfig;

uint32_t termCol = 0;
uint32_t termRow = TERM_ROWS - 1;
/// @brief When reaching the end of a row, move to the next automatically?
bool wrap = true;

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
    channel_config_set_transfer_data_size(&termChan, DMA_SIZE_8);
    channel_config_set_read_increment(&termChan, true);
    channel_config_set_write_increment(&termChan, true);

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
    clearRow(TERM_ROWS - 1);
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
        clearRow(row);
    else
        fillRect(0, row * 8, (col + 1) * 8, 8, BLACK); // TODO: DMA?
}

/// @brief Clears from the end of row, to and including col.
void term_clear_from_end(uint32_t row, uint32_t col) {
    if (row >= TERM_ROWS) return;
    if (col >= TERM_COLS) return;

    if (col == 0)
        clearRow(row);
    else
        fillRect(col * 8, row * 8, WIDTH - col * 8, 8, BLACK); // TODO: DMA?
}

void term_process(char input) {
    if (input == '\033') { // ESC

    }
    else if (input == '\n') {
        termRow++;
        if (termRow >= TERM_ROWS) {
            term_shift_up();
            termRow = TERM_ROWS - 1;
        }
    }
    else if (input == '\r') {
        termCol = 0;
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
        drawChar(termCol * 8, termRow * 8, input, WHITE, BLACK, 1);
        advance();
    }
}
