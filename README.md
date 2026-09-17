# Fall Detection & Alert System

An ESP32-S3 wearable/room device that classifies body state (standing, sitting, walking, dizzy, fall) from MPU6050 motion data using an on-device TinyML model, gives silent per-state Neopixel feedback, sounds a buzzer for concerning states, and sends a Telegram alert to an emergency contact when a fall is confirmed.

**Status:** Phase 2 — GitHub Project Setup

> This README is being filled in phase by phase as the project is built — see `CLAUDE.md` and `docs/TASKS.md`. Sections below will fill in as each phase completes.

**Repo:** [github.com/msohdy/fall-detection-physical-ai-esp32](https://github.com/msohdy/fall-detection-physical-ai-esp32) · **Project board:** [Fall Detection Build Roadmap](https://github.com/users/msohdy/projects/3)

## Overview

*(Problem, goals, and non-goals — filled in from `docs/Fall_Detection_PRD.md`.)*

## Bill of Materials

*(Filled in during Phase 1/3 — components and where to get them.)*

## Wiring

*(Filled in during Phase 3 — pin table, diagram, and build photos.)*

## Setup

**Firmware toolchain:** [PlatformIO](https://platformio.org/) as a VS Code extension.

- Board: `esp32s3usbotg` in `platformio.ini` (PlatformIO's board ID for Espressif's official ESP32-S3-USB-OTG devkit). The actual hardware is a generic ESP32-S3-N16R8 dev board (confirmed via `esptool flash_id`: ESP32-S3, 16MB quad-I/O flash, 8MB quad PSRAM) — see [Build Log / Decisions](#build-log--decisions).
- Framework: `arduino`
- Confirmed: project builds and uploads over USB (native USB port, shows up as `VID:PID=303A:1001`).

## Training the Model

*(Filled in during Phase 4 — dataset, class list, and how to reproduce training.)*

## Building & Flashing

*(Filled in during Phase 6.)*

## Testing & Validation

*(Filled in during Phase 7 — results against the PRD's success criteria.)*

## Known Limitations

*(Filled in during Phase 8 — be specific and honest here.)*

## Build Log / Decisions

*(A running record of meaningful choices and deviations from the original plan, added as they happen — see `CLAUDE.md`.)*

- **License:** MIT, chosen over Apache-2.0 for simplicity (no patent-grant needs expected for a hobbyist hardware project).
- **PlatformIO board ID:** `platformio.ini` uses `board = esp32s3usbotg`, which is PlatformIO's ID for Espressif's *official* ESP32-S3-USB-OTG devkit (LCD + 4 physical buttons). The actual hardware is a generic ESP32-S3-N16R8 dev board with none of that — confirmed by asking the chip directly via `esptool flash_id`: ESP32-S3, 16MB quad-I/O flash, 8MB quad PSRAM. This board ID was kept intentionally despite the mismatch; it builds and uploads fine, but its pin macros won't match this hardware, so **Phase 3 wiring will need pins set explicitly in `include/pins.h` rather than relying on board-default pin names**.
- **Serial-over-USB gotcha (noted for later, not yet applied):** this board has one native USB port (no separate CH340/CP2102 UART bridge), enumerating as `VID:PID=303A:1001`. During bring-up testing, Arduino's `Serial` didn't route through that port until `-DARDUINO_USB_CDC_ON_BOOT=1` was added to `build_flags` — without it, `Serial.print()` silently goes to unused physical UART0 pins instead. The current `main.cpp` doesn't use `Serial` yet, so this flag isn't in `platformio.ini` yet; add it when serial output is actually needed.
- **GitHub Issues/Projects granularity:** one GitHub Issue was created per phase (not per individual task) and attached to a matching milestone, so the [Project board](https://github.com/users/msohdy/projects/3) stays readable. `docs/TASKS.md` remains the source of truth for task-level detail — each phase issue links back to its section there.
- **Secrets:** Telegram bot token/chat ID are currently saved locally in a git-ignored `.env` as personal notes. They'll move into a git-ignored `include/secrets.h` (with a `secrets.h.example` template committed) when Telegram integration is coded in Phase 5 — `.env` isn't read by the firmware.
- **Deferred (by choice, not forgotten):** `CODEOWNERS`/PR-review notes and GitHub Discussions — both explicitly pushed to closer to Phase 7/8 when the project has outside contributors.

## License

MIT — see [`LICENSE`](LICENSE).
