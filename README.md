# Frotz for Adafruit Fruit Jam

This repository is a public-source candidate for playing text-only Z-machine stories on Adafruit Fruit Jam (RP2350B). It combines Frotz's common and dumb-interpreter sources with an SD story picker, 40×22 text display, and USB or on-screen keyboard input. It targets the resident loader's application partition, not a standalone full-flash UF2.

The earlier 0.2.9 no-flicker build passed a basic playability and soft-keyboard retest on development hardware, as confirmed by the user. This repository retargets that source to Fruit Jam. It builds and passes UF2 layout checks, but **Fruit Jam hardware has not been retested**. No tested Fruit Jam binary release is claimed here.

## Stories and controls

Place your own lawfully obtained `.z1` through `.z8` story files in `/roms/FROTZ/` on the SD card. A single matching story is selected automatically; with multiple stories, the picker is shown. No story or user save is included.

For on-screen input, D-pad moves the keyboard cursor, A adds a character, B deletes, X adds a space, Y enters `look`, and Start submits the line. The frontend also supports USB keyboard input. This build disables graphics, sound, true color, and Blorb handling. Save/restore compatibility across stories has not been fully validated.

## Build

Install CMake, Ninja, an ARM GCC toolchain, Pico SDK 2.3.0 or newer, and [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB). Then run:

```powershell
./scripts/build-release.ps1 -PicoSdkPath C:\path\to\pico-sdk -PicoPioUsbPath C:\path\to\Pico-PIO-USB -ToolchainPath C:\path\to\toolchain -NinjaPath C:\path\to\ninja.exe
```

Optional `-PioasmDir`, `-PicotoolDir`, and `-TinyUsbPath` parameters select prebuilt host tools or a TinyUSB checkout. Output: `build/fruit-jam-release/picoFrotz.uf2`. The build targets Fruit Jam, its 8 MiB PSRAM, and DVI video without HDMI audio. Actual Fruit Jam video, USB input, SD access, and clock stability still require physical validation.

The UF2 starts at `0x10080000`, after the resident loader. Do **not** flash it as a standalone bootloader image. Verify its layout with:

```powershell
python ./scripts/validate-uf2-layout.py ./build/fruit-jam-release/picoFrotz.uf2 --app-base 0x10080000 --metadata 0x10FFFF00
```

## Source and license

The interpreter is [DavidGriffith/frotz](https://gitlab.com/DavidGriffith/frotz) commit `64127b159fe4cb5ac4e989428d335836db39cd23`, GPL-2.0-or-later. Selected upstream `src/common/` and `src/dumb/` files are in `deps/frotz/`, with `COPYING` retained. Only `src/dumb/dinput.c` is changed from that commit: its `os_tick()` calls the embedded USB service behind `JACK_FROTZ_EMBEDDED_USB_TICK`. The project-owned `firmware/` and `platform/` files supply the FatFs/stdio bridge, on-screen keyboard, text console, board runtime, and embedded configuration. [THIRD_PARTY.md](THIRD_PARTY.md) lists all copied components and licenses. The combined firmware is distributed under GPLv3 because it also includes GPLv3 components; Frotz retains its GPL-2.0-or-later terms. Building or running the project does not grant rights to distribute story files.
