
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "defs.h"
#include "ff.h"
#include "jack_wave1_runtime.h"
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "text_console.h"
#include "hardware/uart.h"
#include "jack_wave1_perf.h"

extern "C" int frotz_main(int argc, char **argv);

namespace {
constexpr unsigned kFileSlots = 8;
constexpr char kKeys[4][11] = {
    "abcdefghij", "klmnopqrst", "uvwxyz0123", "456789.,-?",
};

struct FileSlot { FIL file; bool used; };
FileSlot g_files[kFileSlots]{};
jack_text_console_t g_console;
char g_input[INPUT_BUFFER_SIZE];
unsigned g_input_length = 0;
unsigned g_input_offset = 0;
bool g_console_ready = false;
bool g_screen_capture_enabled = false;
JackWave1Perf g_perf;
stdio_driver_t g_screen_stdio{};
uint16_t g_buttons = 0;

// jack_wave1_init already owns UART setup.
void serial_write(const char *data, size_t count)
{
    uart_write_blocking(uart0, reinterpret_cast<const uint8_t *>(data), count);
}
void serial_log(const char *line) { serial_write(line, strlen(line)); }

void draw_keyboard_region(const char *edit, unsigned key_x, unsigned key_y)
{
    const uint16_t black = 0x0000;
    const uint16_t white = 0x7fff;
    const uint16_t cyan = 0x03ff;
    const uint16_t panel = 0x0842;
    jack_wave1_fill_rect(0, 176, 320, 64, panel);
    jack_wave1_draw_text(0, 176, edit == nullptr ? "" : edit, cyan, panel);
    for (unsigned y = 0; y < 4u; ++y) {
        for (unsigned x = 0; x < 10u; ++x) {
            const bool selected = x == key_x && y == key_y;
            char label[2] = {kKeys[y][x], '\0'};
            jack_wave1_draw_text(x * 24u, 192u + y * 12u, label,
                                 selected ? black : white,
                                 selected ? cyan : panel);
        }
    }
    jack_wave1_draw_text(248, 192, "A:add", white, panel);
    jack_wave1_draw_text(248, 204, "B:del", white, panel);
    jack_wave1_draw_text(248, 216, "X:spc", white, panel);
    jack_wave1_draw_text(248, 228, "START:go", white, panel);
}

void render(bool show_keyboard, unsigned key_x = 0, unsigned key_y = 0,
            const char *edit = nullptr)
{
    const uint32_t wait_start = time_us_32();
    jack_wave1_wait_vsync();
    const uint32_t draw_start = time_us_32();
    const uint16_t black = 0x0000;
    const uint16_t white = 0x7fff;
    jack_wave1_clear(black);
    for (unsigned row = 0; row < g_console.rows; ++row) {
        for (unsigned col = 0; col < g_console.cols; ++col) {
            const jack_text_cell_t *cell = jack_text_cell(&g_console, col, row);
            uint16_t fg = (cell->style & JACK_TEXT_STYLE_REVERSE) != 0u ? black : white;
            uint16_t bg = (cell->style & JACK_TEXT_STYLE_REVERSE) != 0u ? white : black;
            jack_wave1_draw_char(col * 8u, row * 8u, static_cast<char>(cell->ch), fg, bg);
        }
    }
    if (show_keyboard) draw_keyboard_region(edit, key_x, key_y);
    const uint32_t draw_end = time_us_32();
    g_perf.sample("FROTZ", 0, draw_end - draw_start, draw_start - wait_start,
                  g_buttons, show_keyboard ? "waiting_input" : "output", serial_log);
}

void refresh_keyboard(const char *edit, unsigned key_x, unsigned key_y)
{
    const uint32_t wait_start = time_us_32();
    jack_wave1_wait_vsync();
    const uint32_t draw_start = time_us_32();
    draw_keyboard_region(edit, key_x, key_y);
    const uint32_t draw_end = time_us_32();
    g_perf.sample("FROTZ", 0, draw_end - draw_start, draw_start - wait_start,
                  g_buttons, "waiting_input", serial_log);
}

void collect_line()
{
    const bool capture_was_enabled = g_screen_capture_enabled;
    g_screen_capture_enabled = false;
    char edit[INPUT_BUFFER_SIZE]{};
    unsigned length = 0;
    unsigned key_x = 0;
    unsigned key_y = 0;
    uint16_t previous = 0;
    render(true, key_x, key_y, edit);
    for (;;) {
        const uint16_t buttons = jack_wave1_poll_buttons();
        g_buttons = buttons;
        const uint16_t pressed = static_cast<uint16_t>(buttons & ~previous);
        previous = buttons;
        if (pressed != 0u) {
            char input_log[128];
            snprintf(input_log, sizeof(input_log),
                     "FROTZ_INPUT context=keyboard buttons=%04x pressed=%04x reports=%lu x=%u y=%u\n",
                     buttons, pressed,
                     static_cast<unsigned long>(jack_wave1_usb_report_count()),
                     key_x, key_y);
            serial_log(input_log);
        }

        bool changed = false;
        if ((pressed & JACK_BUTTON_LEFT) != 0u) {
            key_x = (key_x + 9u) % 10u; changed = true;
        }
        if ((pressed & JACK_BUTTON_RIGHT) != 0u) {
            key_x = (key_x + 1u) % 10u; changed = true;
        }
        if ((pressed & JACK_BUTTON_UP) != 0u) {
            key_y = (key_y + 3u) % 4u; changed = true;
        }
        if ((pressed & JACK_BUTTON_DOWN) != 0u) {
            key_y = (key_y + 1u) % 4u; changed = true;
        }
        if ((pressed & JACK_BUTTON_A) != 0u && length + 1u < sizeof(edit)) {
            edit[length++] = kKeys[key_y][key_x]; edit[length] = '\0'; changed = true;
        }
        if ((pressed & JACK_BUTTON_X) != 0u && length + 1u < sizeof(edit)) {
            edit[length++] = ' '; edit[length] = '\0'; changed = true;
        }
        if ((pressed & JACK_BUTTON_B) != 0u && length != 0u) {
            edit[--length] = '\0'; changed = true;
        }
        if ((pressed & JACK_BUTTON_Y) != 0u) {
            strcpy(edit, "look"); length = 4; changed = true;
        }
        if ((pressed & JACK_BUTTON_START) != 0u) break;
        if (changed) refresh_keyboard(edit, key_x, key_y);
        g_perf.sample("FROTZ", 0, 0, 0, g_buttons,
                      "waiting_input", serial_log);
        sleep_ms(16);
    }
    serial_log("FROTZ_INPUT submitted\n");
    memcpy(g_input, edit, length);
    g_input[length++] = '\n';
    g_input_length = length;
    g_input_offset = 0;
    for (unsigned i = 0; i < length; ++i) jack_text_putc(&g_console, g_input[i]);
    render(false);
    jack_wave1_wait_buttons_released();
    g_screen_capture_enabled = capture_was_enabled;
}
void screen_stdio_out_chars(const char *buffer, int count)
{
    if (!g_console_ready || !g_screen_capture_enabled || buffer == nullptr || count <= 0) return;
    bool refresh = false;
    for (int i = 0; i < count; ++i) {
        jack_text_putc(&g_console, static_cast<uint8_t>(buffer[i]));
        refresh = refresh || buffer[i] == '\n' || g_console.cursor_col == 0;
    }
    if (refresh) render(false);
}

int screen_stdio_in_chars(char *buffer, int count)
{
    if (buffer == nullptr || count <= 0) return 0;
    if (g_input_offset == g_input_length) collect_line();
    const unsigned available = g_input_length - g_input_offset;
    const unsigned requested = static_cast<unsigned>(count);
    const unsigned take = available < requested ? available : requested;
    memcpy(buffer, g_input + g_input_offset, take);
    g_input_offset += take;
    return static_cast<int>(take);
}

void enable_screen_stdio()
{
    g_screen_stdio.out_chars = screen_stdio_out_chars;
    g_screen_stdio.in_chars = screen_stdio_in_chars;
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    g_screen_stdio.crlf_enabled = false;
#endif
    g_screen_capture_enabled = true;
    stdio_set_driver_enabled(&g_screen_stdio, true);
}

BYTE fatfs_mode(int flags)
{
    BYTE mode = (flags & O_RDWR) ? (FA_READ | FA_WRITE) :
                (flags & O_WRONLY) ? FA_WRITE : FA_READ;
    if ((flags & O_CREAT) != 0) mode |= (flags & O_TRUNC) ? FA_CREATE_ALWAYS : FA_OPEN_ALWAYS;
    return mode;
}
}

extern "C" void frotz_embedded_usb_tick(void)
{
    static uint32_t next_service = 0;
    const uint32_t now = time_us_32();
    if (static_cast<int32_t>(now - next_service) >= 0) {
        const bool capture_was_enabled = g_screen_capture_enabled;
        g_screen_capture_enabled = false;
        jack_wave1_usb_service();
        g_screen_capture_enabled = capture_was_enabled;
        next_service = now + 750u;
    }
}

extern "C" int _open(const char *path, int flags, ...)
{
    for (unsigned i = 0; i < kFileSlots; ++i) {
        if (!g_files[i].used && f_open(&g_files[i].file, path, fatfs_mode(flags)) == FR_OK) {
            g_files[i].used = true;
            if ((flags & O_APPEND) != 0) (void)f_lseek(&g_files[i].file, f_size(&g_files[i].file));
            return static_cast<int>(i + 3u);
        }
    }
    errno = ENOENT;
    return -1;
}

extern "C" int _close(int fd)
{
    if (fd < 3 || fd >= static_cast<int>(3u + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    const FRESULT result = f_close(&g_files[fd - 3].file);
    g_files[fd - 3].used = false;
    return result == FR_OK ? 0 : -1;
}

extern "C" int _read(int fd, char *buffer, int count)
{
    if (fd == 0) {
        if (g_input_offset == g_input_length) collect_line();
        const unsigned available = g_input_length - g_input_offset;
        const unsigned take = available < static_cast<unsigned>(count) ? available : count;
        memcpy(buffer, g_input + g_input_offset, take);
        g_input_offset += take;
        return static_cast<int>(take);
    }
    if (fd < 3 || fd >= static_cast<int>(3u + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    UINT read = 0;
    return f_read(&g_files[fd - 3].file, buffer, count, &read) == FR_OK ?
        static_cast<int>(read) : -1;
}

extern "C" int _write(int fd, const char *buffer, int count)
{
    if (fd == 1 || fd == 2) {
        if (count <= 0) return 0;
        serial_write(buffer, static_cast<size_t>(count));
        if (!g_console_ready) return count;
        for (int i = 0; i < count; ++i) jack_text_putc(&g_console, static_cast<uint8_t>(buffer[i]));
        if (count != 0 && (buffer[count - 1] == '\n' || g_console.cursor_col == 0)) render(false);
        return count;
    }
    if (fd < 3 || fd >= static_cast<int>(3u + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    UINT written = 0;
    return f_write(&g_files[fd - 3].file, buffer, count, &written) == FR_OK ?
        static_cast<int>(written) : -1;
}

extern "C" int _lseek(int fd, int offset, int whence)
{
    if (fd < 3 || fd >= static_cast<int>(3u + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    FIL &file = g_files[fd - 3].file;
    FSIZE_t position = whence == SEEK_SET ? offset :
                       whence == SEEK_CUR ? f_tell(&file) + offset : f_size(&file) + offset;
    if (f_lseek(&file, position) != FR_OK) return -1;
    return static_cast<int>(f_tell(&file));
}

extern "C" int _fstat(int fd, struct stat *status)
{
    memset(status, 0, sizeof(*status));
    status->st_mode = fd <= 2 ? S_IFCHR : S_IFREG;
    if (fd >= 3 && fd < static_cast<int>(3u + kFileSlots) && g_files[fd - 3].used)
        status->st_size = static_cast<off_t>(f_size(&g_files[fd - 3].file));
    return 0;
}
extern "C" int _isatty(int fd) { return fd >= 0 && fd <= 2; }

int main()
{
    if (!jack_wave1_init("Frotz Z-machine")) for (;;) tight_loop_contents();
    printf("FROTZ_STAGE main_enter\n");
    serial_log("FROTZ_STAGE runtime_ready\n");
    jack_text_init(&g_console, 40, 22);
    g_perf.begin();
    serial_log("FROTZ_BOOT version=0.2.9-fruit-jam uart=0 tx=44 rx=45 baud=115200 undo=8 max_undo=8\n");
    if (!jack_wave1_mount_sd()) {
        jack_wave1_show_error("FROTZ", "SD mount failed");
        for (;;) tight_loop_contents();
    }
    char story[192];
    jack_wave1_set_picker_auto_select_single(true);
    if (!jack_wave1_select_file("SELECT Z-MACHINE STORY", "/roms/FROTZ",
                                ".z1 .z2 .z3 .z4 .z5 .z6 .z7 .z8",
                                story, sizeof(story))) {
        jack_wave1_show_error("FROTZ", "No story in /roms/FROTZ");
        for (;;) tight_loop_contents();
    }
    serial_log("FROTZ_STAGE story_selected\n");
    g_console_ready = true;
    enable_screen_stdio();
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    char program[] = "frotz";
    char width_flag[] = "-w";
    char width[] = "40";
    char height_flag[] = "-h";
    char height[] = "22";
    char *argv[] = {program, width_flag, width, height_flag, height, story, nullptr};
    serial_log("FROTZ_STAGE interpreter_start\n");
    g_perf.begin();
    return frotz_main(6, argv);
}
