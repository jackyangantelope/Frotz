#include "text_console.h"

#include <string.h>

static void clear_row(jack_text_console_t *console, uint8_t row)
{
    for (uint8_t col = 0; col < console->cols; ++col) {
        console->cells[row][col].ch = (uint8_t)' ';
        console->cells[row][col].style = JACK_TEXT_STYLE_ROMAN;
    }
}

static void scroll(jack_text_console_t *console)
{
    const uint8_t top = console->scroll_top;
    const uint8_t bottom = console->scroll_bottom;

    for (uint8_t row = top; row < bottom; ++row) {
        memcpy(console->cells[row], console->cells[row + 1],
               sizeof(console->cells[row]));
    }
    clear_row(console, bottom);
    console->cursor_row = bottom;
    ++console->scroll_count;
}

static void newline(jack_text_console_t *console)
{
    console->cursor_col = 0;
    if (console->cursor_row < console->scroll_bottom) {
        ++console->cursor_row;
    } else {
        scroll(console);
    }
}

void jack_text_init(jack_text_console_t *console, uint8_t cols, uint8_t rows)
{
    if (console == NULL) {
        return;
    }
    memset(console, 0, sizeof(*console));
    console->cols = (cols == 0 || cols > JACK_TEXT_MAX_COLS) ? JACK_TEXT_MAX_COLS : cols;
    console->rows = (rows == 0 || rows > JACK_TEXT_MAX_ROWS) ? JACK_TEXT_MAX_ROWS : rows;
    console->scroll_bottom = (uint8_t)(console->rows - 1u);
    jack_text_clear(console);
}

void jack_text_clear(jack_text_console_t *console)
{
    if (console == NULL || console->cols == 0 || console->rows == 0) {
        return;
    }
    for (uint8_t row = 0; row < console->rows; ++row) {
        clear_row(console, row);
    }
    console->cursor_col = 0;
    console->cursor_row = console->scroll_top;
    console->scroll_count = 0;
}

void jack_text_set_style(jack_text_console_t *console, uint8_t style)
{
    if (console != NULL) {
        console->style = style;
    }
}

void jack_text_set_cursor(jack_text_console_t *console, uint8_t col, uint8_t row)
{
    if (console == NULL || col >= console->cols || row >= console->rows) {
        return;
    }
    console->cursor_col = col;
    console->cursor_row = row;
}

void jack_text_set_scroll_region(jack_text_console_t *console, uint8_t top, uint8_t bottom)
{
    if (console == NULL || top > bottom || bottom >= console->rows) {
        return;
    }
    console->scroll_top = top;
    console->scroll_bottom = bottom;
    if (console->cursor_row < top || console->cursor_row > bottom) {
        console->cursor_row = top;
        console->cursor_col = 0;
    }
}

void jack_text_putc(jack_text_console_t *console, uint8_t ch)
{
    if (console == NULL || console->cols == 0 || console->rows == 0) {
        return;
    }

    if (ch == (uint8_t)'\r') {
        console->cursor_col = 0;
        return;
    }
    if (ch == (uint8_t)'\n') {
        newline(console);
        return;
    }
    if (ch == (uint8_t)'\t') {
        const uint8_t next = (uint8_t)((console->cursor_col + 4u) & ~3u);
        do {
            jack_text_putc(console, (uint8_t)' ');
        } while (console->cursor_col != 0 && console->cursor_col < next);
        return;
    }
    if (ch < 0x20u) {
        return;
    }

    jack_text_cell_t *cell = &console->cells[console->cursor_row][console->cursor_col];
    cell->ch = ch;
    cell->style = console->style;

    ++console->cursor_col;
    if (console->cursor_col >= console->cols) {
        newline(console);
    }
}

void jack_text_write(jack_text_console_t *console, const char *text)
{
    if (text == NULL) {
        return;
    }
    while (*text != '\0') {
        jack_text_putc(console, (uint8_t)*text++);
    }
}

const jack_text_cell_t *jack_text_cell(const jack_text_console_t *console,
                                       uint8_t col,
                                       uint8_t row)
{
    if (console == NULL || col >= console->cols || row >= console->rows) {
        return NULL;
    }
    return &console->cells[row][col];
}
