# Fall Detection & Alert System

An ESP32-S3 wearable/room device that classifies body state (standing, sitting, walking, dizzy, fall) from MPU6050 motion data using an on-device TinyML model, gives silent per-state Neopixel feedback, sounds a buzzer for concerning states, and sends a Telegram alert to an emergency contact when a fall is confirmed.

**Status:** Phase 6 — Build, Flash & Upload

> This README is being filled in phase by phase as the project is built — see `CLAUDE.md` and `docs/TASKS.md`. Sections below will fill in as each phase completes.

**Repo:** [github.com/msohdy/fall-detection-physical-ai-esp32](https://github.com/msohdy/fall-detection-physical-ai-esp32) · **Project board:** [Fall Detection Build Roadmap](https://github.com/users/msohdy/projects/3)

## Overview

*(Problem, goals, and non-goals — filled in from `docs/Fall_Detection_PRD.md`.)*

## Bill of Materials

*(Filled in during Phase 1/3 — components and where to get them.)*

## Wiring

**Diagram tool:** [Fritzing](https://fritzing.org/) (free, open source, breadboard-style diagrams; switched from an initial Wokwi plan once actually building the diagram).

![Breadboard wiring diagram](docs/wiring/fall-detection-wiring.png)

Fritzing project source: [`docs/wiring/fall-detection-wiring.fzz`](docs/wiring/fall-detection-wiring.fzz)

**Actual build:**

![Breadboard photo](docs/wiring/wiringImage.jpg)

**Pin table:**

| Signal | MPU6050 | Neopixel (onboard) | Buzzer (active) |
| --- | --- | --- | --- |
| Power | VCC → 3.3V | onboard, no wiring | + → **GPIO6** (switched power) |
| Ground | GND → GND | onboard | - → GND |
| Data/Signal | SDA → GPIO8 | GPIO48 (onboard) | *(none — see note below)* |
| Clock | SCL → GPIO9 | — | — |

The Neopixel is the board's onboard single WS2812 RGB LED, not an external strip. The buzzer is confirmed **active** with no dedicated signal pin at all — its 3rd pin is unused, and it's controlled by switching power to it directly via GPIO6 (`digitalWrite(HIGH/LOW)`), not `tone()`. See [Build Log / Decisions](#build-log--decisions) for how this was determined.

**Form factor (v1):** breadboard prototype, powered by a USB powerbank — handheld, not worn or room-mounted. No enclosure/form-factor commitment yet; strain relief and wearable-specific concerns are out of scope until a final form factor is chosen.

*(Wiring diagram export and breadboard photos to be added here once produced.)*

## Setup

**Firmware toolchain:** [PlatformIO](https://platformio.org/) as a VS Code extension.

- Board: `esp32s3usbotg` in `platformio.ini` (PlatformIO's board ID for Espressif's official ESP32-S3-USB-OTG devkit). The actual hardware is a generic ESP32-S3-N16R8 dev board (confirmed via `esptool flash_id`: ESP32-S3, 16MB quad-I/O flash, 8MB quad PSRAM) — see [Build Log / Decisions](#build-log--decisions).
- Framework: `arduino`
- Confirmed: project builds and uploads over USB (native USB port, shows up as `VID:PID=303A:1001`).

## Data Collection Tooling

**[`tools/serial-data-logger.html`](tools/serial-data-logger.html)** — a self-contained Web Serial page (Chrome/Edge only) for capturing MPU6050 samples to CSV. Built after two Edge Impulse ingestion paths turned out to be dead ends on this setup:

1. `edge-impulse-cli` (for the classic Data Forwarder) failed to install — one dependency needs native compilation via Visual Studio Build Tools, which aren't installed.
2. Edge Impulse Studio's browser-based "Connect a new device" WebSerial flow requires the device firmware to speak Edge Impulse's AT-command protocol — a different thing entirely from plain CSV serial output, and not worth implementing just for data collection.

Connect to the board, log incoming CSV lines to an editable table (delete bad rows individually or in bulk, or reopen an already-saved CSV via **Load CSV**), and export a clean CSV per recording session.

![Serial Data Logger](tools/logger-screenshot.png)

See [`tools/README.md`](tools/README.md) for full usage instructions and the data collection + training pipeline diagram.

## Training the Model

Edge Impulse was dropped entirely as the training platform too (not just the data-connection method above) — see [Build Log / Decisions](#build-log--decisions). Instead: **[`tools/tinyml-trainer.html`](tools/tinyml-trainer.html)**, a self-contained TensorFlow.js page that trains a small dense-network classifier client-side and exports a plain C header for firmware.

![TinyML Model Trainer](tools/trainer-screenshot.png)

**Dataset:** 8116 raw samples across 5 classes (fall 960, sit 1900, stand 1399, walk 1971, dizzy 1886), collected handheld (see Wiring's form-factor note) via `serial-data-logger.html`.

**Model:** `dense(16, relu) → dense(8, relu) → dense(5, softmax)`, trained on windowed raw accel+gyro samples (no DSP/spectral feature extraction). Final: window size 75 samples (~1.5s at 50Hz), 50 epochs, learning rate 0.001, train accuracy 0.999, validation accuracy 0.963.

**Window size mattered a lot.** Started at window=5 (100ms) — far shorter than the PRD's suggested ~1-2s — and it showed:

| Window size | Duration @ 50Hz | Overall val accuracy | Fall samples missed |
| --- | --- | --- | --- |
| 5 | 0.1s | ~0.87 | 68/209 (32.5%) |
| 50 | 1.0s | 0.958 | 39/191 (20.4%) |
| 75 | 1.5s | 0.963 | 23/205 (11.2%) — accepted for v1 |

**Known limitation:** the PRD's "zero missed falls" bar is not fully met — 11.2% of fall validation samples are still confused with "dizzy," which is more likely a data-realism issue (handheld mimicked falls vs. a real worn fall's sharper signature) than something further hyperparameter tuning alone would fix. See [Known Limitations](#known-limitations).

Full model header: [`include/fall_detection_model_data.h`](include/fall_detection_model_data.h). See [`tools/README.md`](tools/README.md) for the trainer's usage instructions and a real bug (a `validationSplit` shuffling gotcha) worth knowing about if you extend this tool.

## Firmware Architecture

**Deviation from the PRD:** no TFLite Micro. Since training moved to a custom in-browser trainer (above) that exports raw float weight/bias arrays instead of a `.tflite` file, the firmware does inference by hand instead of via a TFLite Micro runtime.

| Module | Files | Responsibility |
| --- | --- | --- |
| MPU6050 driver | `mpu6050.h/.cpp` | Raw I2C register reads (same approach as the Phase 4 data-forwarder sketch), scaled to match training data exactly |
| Model inference | `model_inference.h/.cpp` | Hand-rolled `dense(450→16, ReLU) → dense(16→8, ReLU) → dense(8→5) → argmax` forward pass over `fall_detection_model_data.h` |
| Neopixel driver | `neopixel_driver.h/.cpp` | `VisualState` enum; steady colors for standing/sitting/walking/dizzy, non-blocking pulse/fade animation for alarmed/recovering |
| Buzzer driver | `buzzer_driver.h/.cpp` | `BuzzerState` enum; distinct non-blocking on/off beep *patterns* per state (this buzzer can't vary pitch — see Wiring section) |
| State machine | `state_machine.h/.cpp` | NORMAL/DIZZY/ALARMED/RECOVERING transitions per the PRD, using string comparison against model labels (not hardcoded indices) |
| Telegram | `telegram.h/.cpp` | HTTPS GET to the Bot API, non-blocking WiFi check, retries the fall alert automatically if WiFi wasn't ready on the first attempt |

**Classification runs on a fresh, non-overlapping ~1.5s window per cycle**, not continuously — see [Known Limitations](#known-limitations) for why continuous classification made false alarms dramatically worse.

**Confirmed working end-to-end**: a simulated fall correctly triggers the siren/red-pulse, sends a real Telegram alert, and (with the right sequence of readings) recovers back to normal.

See `docs/TASKS.md` (Phase 5) for the full list of deviations and bugs found/fixed along the way (a linker "multiple definition" gotcha from the generated model header, the false-alarm classification-rate fix, and an open follow-up on the recovery-exit criteria interacting with real model behavior).

## Building & Flashing

*(Filled in during Phase 6.)*

## Testing & Validation

*(Filled in during Phase 7 — results against the PRD's success criteria.)*

## Known Limitations

*(Full writeup during Phase 8 — noting the significant one early since it's directly measured, not speculative.)*

- **Fall detection does not fully meet "zero missed falls" yet.** The trained model misses ~11.2% of fall validation samples (23/205), almost all confused with "dizzy." This is very likely tied to the handheld (not worn) form factor — a real fall experienced by a worn device has a sharper acceleration/rotation signature than a handheld mimicked fall motion, making "fall" and "dizzy" genuinely harder to tell apart in this dataset. Revisit with a wearable form factor and/or more/cleaner fall data.
- **Occasional false alarms are expected, by design trade-off.** The model also has a small non-fall→fall misclassification rate (~1.2%, mostly dizzy→fall). Firmware classifies once per ~1.5s window (not continuously), which keeps this down to roughly one false alarm every couple of minutes at rest — acceptable per the PRD's explicit priority (missed falls matter more than false positives), but worth knowing before real-world deployment. See Phase 5 notes in `docs/TASKS.md` for the math on why continuous (sliding-window) classification made this dramatically worse and was reverted.

## Build Log / Decisions

*(A running record of meaningful choices and deviations from the original plan, added as they happen — see `CLAUDE.md`.)*

- **License:** MIT, chosen over Apache-2.0 for simplicity (no patent-grant needs expected for a hobbyist hardware project).
- **PlatformIO board ID:** `platformio.ini` uses `board = esp32s3usbotg`, which is PlatformIO's ID for Espressif's *official* ESP32-S3-USB-OTG devkit (LCD + 4 physical buttons). The actual hardware is a generic ESP32-S3-N16R8 dev board with none of that — confirmed by asking the chip directly via `esptool flash_id`: ESP32-S3, 16MB quad-I/O flash, 8MB quad PSRAM. This board ID was kept intentionally despite the mismatch; it builds and uploads fine, but its pin macros won't match this hardware, so **Phase 3 wiring will need pins set explicitly in `include/pins.h` rather than relying on board-default pin names**.
- **Serial-over-USB gotcha:** this board has one native USB port (no separate CH340/CP2102 UART bridge), enumerating as `VID:PID=303A:1001`. Arduino's `Serial` doesn't route through that port by default — without `-DARDUINO_USB_CDC_ON_BOOT=1` in `build_flags`, `Serial.print()` silently goes to unused physical UART0 pins instead. This flag is now set in `platformio.ini` (added once `main.cpp` needed serial output for I2C bring-up testing).
- **GitHub Issues/Projects granularity:** one GitHub Issue was created per phase (not per individual task) and attached to a matching milestone, so the [Project board](https://github.com/users/msohdy/projects/3) stays readable. `docs/TASKS.md` remains the source of truth for task-level detail — each phase issue links back to its section there.
- **Secrets:** moved from the Phase 1 `.env` notes into a proper git-ignored `include/secrets.h` (with `secrets.h.example` committed as the template) in Phase 5. The `.env`'s saved "chat ID" turned out to actually be the `getUpdates` *lookup URL*, not the real numeric chat ID — had to message the bot and call that URL for real to get the actual ID.
- **Telegram HTTPS uses `WiFiClientSecure::setInsecure()`** (no certificate pinning) — a common simplification for a hobbyist project, at the cost of no protection against a MITM on the local network. A future hardening pass could embed Telegram's root CA certificate instead.
- **Deferred (by choice, not forgotten):** `CODEOWNERS`/PR-review notes and GitHub Discussions — both explicitly pushed to closer to Phase 7/8 when the project has outside contributors.
- **Buzzer confirmed active** by direct test: connecting VCC to 3.3V and GND with the 3rd pin left unwired produced a steady, continuous tone — meaning it has its own internal driver circuit. This corrects an initial guess of "passive" based on an old sample sketch that used `tone()` with varying frequencies; that sketch apparently doesn't match this actual buzzer.
- **Buzzer has no logic-level control pin at all** — its 3rd pin turned out to be unused (it buzzed with just VCC+GND wired). Control instead works by switching **power** to the buzzer: the wire originally planned for a fixed 3.3V rail goes to **GPIO6** instead, which sources power directly; GND stays on GND; the 3rd/mid pin is left disconnected. Confirmed working via `digitalWrite(GPIO6, HIGH/LOW)`. Since an active buzzer also can't vary pitch, the PRD's "distinct tone per state" requirement will need to become distinct on/off beep *patterns* instead (Phase 5). Driving the buzzer directly off a GPIO (no transistor) works for now but may need revisiting if current draw proves too high for the pin.
- **I2C pins set explicitly rather than relying on board defaults**, since `platformio.ini`'s board ID doesn't exactly match the physical hardware (see the board-ID note above) — GPIO8 (SDA) / GPIO9 (SCL), which happen to match the Arduino-ESP32 core's own `Wire.begin()` defaults on ESP32-S3 anyway.
- **Edge Impulse dropped entirely, not just its data-connection method.** After building `serial-data-logger.html` to work around two Edge Impulse ingestion dead-ends, training itself was also moved off Edge Impulse — a 5-class classifier on raw windowed IMU data doesn't need Edge Impulse's DSP/Spectral Analysis blocks, and a fully local pipeline keeps the project reproducible without an external account. Built `tools/tinyml-trainer.html` (TensorFlow.js, in-browser) instead, exporting a plain C header with raw float weight/bias arrays. This means **Phase 5 firmware will implement inference by hand** (a small forward pass over the exported arrays) rather than integrating the TFLite Micro runtime the PRD originally assumed.
- **Found and fixed a real bug in the training tool**: early runs showed validation accuracy stuck low (~0.3-0.5) and unstable while training accuracy climbed normally — not standard overfitting, but because the windowed dataset was built one class at a time, and TensorFlow.js's `validationSplit` carves its slice off the *end* of the array before any shuffling. The validation set was almost entirely just the last class or two. Fixed by shuffling the windowed samples across classes before training.
- **Window size tuned from 5 to 75 samples** after discovering the initial default (100ms at 50Hz) was far too short to characterize a fall's shape, causing heavy confusion with "dizzy." See [Training the Model](#training-the-model) for the full tuning table and the resulting known limitation.
- **Generated model header must only be included from one `.cpp` file.** `fall_detection_model_data.h` defines `LABELS[]` and the weight/bias arrays without `extern`, so including it from more than one file causes a linker "multiple definition" error (and would otherwise silently duplicate ~160KB of flash per file that includes it). Firmware confines it to `src/model_inference.cpp`; everything else goes through `model_inference.h`, which mirrors the model's dimension macros with a `static_assert` guard so a future retrain with different dimensions fails the build instead of silently misbehaving.
- **Classification runs on a fresh window per cycle, not a continuous sliding window.** An early firmware version classified on every new IMU sample (~50Hz sliding window) for lower latency, but combined with the model's small non-fall→fall misclassification rate (~1.2%, mostly dizzy→fall), that compounded into false ALARMED triggers roughly every few seconds even at rest. Reverted to a fresh, non-overlapping window per classification (~0.67/sec) — this also matches how the model was actually validated (independent windows) — bringing false alarms down to roughly once every couple of minutes. See [Known Limitations](#known-limitations).

## License

MIT — see [`LICENSE`](LICENSE).
