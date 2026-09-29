cmake_minimum_required(VERSION 3.20)
set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Adafruit Fruit Jam" FORCE)
set(PICO_PSRAM_CS_PIN 47 CACHE STRING "Fruit Jam PSRAM chip select" FORCE)
set(PICO_PSRAM_SIZE_BYTES 0x800000 CACHE STRING "Fruit Jam PSRAM size" FORCE)
set(PICO_DEFAULT_UART 0 CACHE STRING "Diagnostic UART" FORCE)
set(PICO_DEFAULT_UART_TX_PIN 44 CACHE STRING "Diagnostic UART TX" FORCE)
set(PICO_DEFAULT_UART_RX_PIN 45 CACHE STRING "Diagnostic UART RX" FORCE)
set(PICO_FLASH_SPI_CLKDIV 10 CACHE STRING "Conservative XIP boot divider" FORCE)
if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to Pico SDK 2.3.0 or newer")
endif()
set(PICO_SHARED_PATH "${CMAKE_CURRENT_LIST_DIR}/deps/pico_shared" CACHE PATH "pico_shared source")
if(NOT DEFINED PICO_PIO_USB_PATH)
    message(FATAL_ERROR "Set PICO_PIO_USB_PATH to a Pico-PIO-USB checkout")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(picoFrotz C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(FROTZ_COMMON "${CMAKE_CURRENT_LIST_DIR}/deps/frotz/src/common")
set(FROTZ_DUMB "${CMAKE_CURRENT_LIST_DIR}/deps/frotz/src/dumb")
set(FROTZ_COMMON_SOURCES
    buffer.c err.c fastmem.c files.c getopt.c hotkey.c input.c main.c math.c
    missing.c object.c process.c quetzal.c random.c redirect.c screen.c sound.c
    stream.c table.c text.c variable.c)
list(TRANSFORM FROTZ_COMMON_SOURCES PREPEND "${FROTZ_COMMON}/")
add_library(frotz_core STATIC ${FROTZ_COMMON_SOURCES}
    "${FROTZ_DUMB}/dinit.c" "${FROTZ_DUMB}/dinput.c"
    "${FROTZ_DUMB}/doutput.c" "${FROTZ_DUMB}/dpic.c")
set_source_files_properties("${FROTZ_COMMON}/main.c" PROPERTIES COMPILE_DEFINITIONS "main=frotz_main")
target_include_directories(frotz_core PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/platform/embedded-include" "${FROTZ_COMMON}" "${FROTZ_DUMB}")
target_compile_definitions(frotz_core PRIVATE NO_BLORB NO_SOUND NO_GRAPHICS NO_TRUECOLOUR JACK_FROTZ_EMBEDDED_USB_TICK)
target_compile_options(frotz_core PRIVATE -Wno-unused-parameter -Wno-sign-compare)

add_library(frotz_text STATIC "${CMAKE_CURRENT_LIST_DIR}/platform/text/text_console.c")
target_include_directories(frotz_text PUBLIC "${CMAKE_CURRENT_LIST_DIR}/platform/text")
set(JACK_WAVE1_HDMI_AUDIO OFF)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/platform/wave1" "${CMAKE_BINARY_DIR}/wave1")
target_compile_definitions(jack_wave1_runtime PRIVATE
    JACK_WAVE1_SYS_CLOCK_KHZ=252000u
    JACK_WAVE1_FLASH_MAX_HZ=42000000u
    JACK_WAVE1_PSRAM_MAX_HZ=96000000u)
target_link_libraries(jack_wave1_runtime PRIVATE hardware_pll hardware_psram)
option(BUILD_FOR_BOOTLOADER "Link to the resident loader app partition" ON)
add_executable(picoFrotz
    "${CMAKE_CURRENT_LIST_DIR}/firmware/playable_platform.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/firmware/basename_compat.c")
pico_set_program_name(picoFrotz "picoFrotz")
pico_set_program_version(picoFrotz "0.2.9-fruit-jam-source-candidate")
pico_enable_stdio_uart(picoFrotz 1)
pico_enable_stdio_usb(picoFrotz 0)
target_compile_definitions(picoFrotz PRIVATE HW_CONFIG=8 PICO_STACK_SIZE=4096
    PICO_FLASH_SPI_CLKDIV=10 PICO_PSRAM_CS_PIN=47 PICO_PSRAM_SIZE_BYTES=0x800000)
target_link_libraries(picoFrotz PRIVATE pico_stdlib hardware_psram frotz_core frotz_text jack_wave1_runtime)
target_link_options(picoFrotz PRIVATE -Wl,--print-memory-usage)
if(BUILD_FOR_BOOTLOADER)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/LoaderPartition.cmake")
    frotz_link_for_loader(picoFrotz)
endif()
pico_add_extra_outputs(picoFrotz)

