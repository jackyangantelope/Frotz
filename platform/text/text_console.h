#ifndef JACK_FROTZ_TEXT_CONSOLE_H
#define JACK_FROTZ_TEXT_CONSOLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JACK_TEXT_MAX_COLS 40u
#define JACK_TEXT_MAX_ROWS 30u

enum jack_text_style {
    JACK_TEXT_STYLE_ROMAN = 0,
    JACK_TEXT_STYLE_REVERSE = 1u << 0,
    JACK_TEXT_STYLE_BOLD = 1u << 1,
    JACK_TEXT_STYLE_ITALIC = 1u << 2,
    JACK_TEXT_STYLE_FIXED = 1u << 3,
};

typedef struct jack_text_cell {
    uint8_t ch;
    uint8_t style;
} jack_text_cell_t;

typedef struct jack_text_console {
    jack_text_cell_t cells[JACK_TEXT_MAX_ROWS][JACK_TEXT_MAX_COLS];
    uint8_t cols;
    uint8_t rows;
    uint8_t cursor_col;
    uint8_t cursor_row;
    uint8_t style;
    uint8_t scroll_top;
    uint8_t scroll_bottom;
    uint32_t scroll_count;
} jack_text_console_t;

void jack_text_init(jack_text_console_t *console, uint8_t cols, uint8_t rows);
void jack_text_clear(jack_text_console_t *console);
void jack_text_set_style(jack_text_console_t *console, uint8_t style);
void jack_text_set_cursor(jack_text_console_t *console, uint8_t col, uint8_t row);
void jack_text_set_scroll_region(jack_text_console_t *console, uint8_t top, uint8_t bottom);
void jack_text_putc(jack_text_console_t *console, uint8_t ch);
void jack_text_write(jack_text_console_t *console, const char *text);
const jack_text_cell_t *jack_text_cell(const jack_text_console_t *console,
                                       uint8_t col,
                                       uint8_t row);

#ifdef __cplusplus
}
#endif

#endif
