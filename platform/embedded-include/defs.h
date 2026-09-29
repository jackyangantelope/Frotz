#ifndef COMMON_DEFINES_H
#define COMMON_DEFINES_H
#define VERSION "2.56pre-fruit-jam"
#define RELEASE_NOTES "Adafruit Fruit Jam embedded text port"
#define MAX_UNDO_SLOTS 8
#define DEFAULT_UNDO_SLOTS 8
#if DEFAULT_UNDO_SLOTS > MAX_UNDO_SLOTS
#error "Default undo slots exceed embedded limit"
#endif
#define MAX_FILE_NAME 192
#define TEXT_BUFFER_SIZE 512
#define INPUT_BUFFER_SIZE 200
#define STACK_SIZE 1024
#define NO_BLORB
#define NO_SOUND
#define NO_GRAPHICS
#define NO_TRUECOLOUR
#define NO_UCONTEXT_H
#define NO_EXECINFO_H
#endif

