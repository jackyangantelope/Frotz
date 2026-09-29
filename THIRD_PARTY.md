# Third-party code and provenance

- [DavidGriffith/frotz](https://gitlab.com/DavidGriffith/frotz), base commit `64127b159fe4cb5ac4e989428d335836db39cd23`: selected upstream `src/common/` and `src/dumb/` C sources and headers in `deps/frotz/`. The only changed upstream file is `src/dumb/dinput.c`, where `os_tick()` calls `frotz_embedded_usb_tick()` when `JACK_FROTZ_EMBEDDED_USB_TICK` is defined. Frotz is GPL-2.0-or-later; its GPLv2 text is retained in `deps/frotz/COPYING`.
- [PicoPlus-devel/pico_shared](https://github.com/PicoPlus-devel/pico_shared), base commit `df683bf00ff95e1ceda88f132bbf72f8cc7cadb0`: minimal public source subset in `deps/pico_shared/`, including the HSTX driver. Its GPLv3 license is retained in `deps/pico_shared/LICENSE`; pico_fatfs retains its BSD-style `deps/pico_shared/drivers/pico_fatfs/LICENSE` and FatFs source-file notices.
- The Fruit Jam `platform/wave1/` runtime, `platform/text/` console, loader-partition helper, and `firmware/` frontend are adapted from the jack-green-ports development workspace and related public [ClassicIF](https://github.com/jackyangantelope/ClassicIF) release. Their shared BSD-3-Clause notice is retained in `LICENSES/jack-green-ports-BSD-3-Clause.txt`. The combined firmware is distributed under GPLv3.
- Pico SDK and Pico-PIO-USB are external build dependencies, not included in this repository. Follow their upstream notices and licenses.

This repository contains no Z-machine story, user save, or generated UF2 binary.
