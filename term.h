void term_init(void);
void term_shift_up(void);
void term_clear_row(uint32_t row);
void term_clear_from_start(uint32_t row, uint32_t col);
void term_clear_from_end(uint32_t row, uint32_t col);
void term_process(char input);
