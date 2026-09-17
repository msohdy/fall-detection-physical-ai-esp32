# Fall Detection System — Build & Open-Source Task List

2026-09-17 · @u_Gd73xkkFmo9snIgr0fdFzg

## Overview

This is the execution checklist for the Fall Detection & Alert System PRD: a room/wearable ESP32-S3 device that classifies body state from an MPU6050 and sends a Telegram alert on a confirmed fall. Work through the seven phases in order — each one hands off what the next needs (wiring before firmware, trained model before flashing, working device before writing it up).

Each task below is a checkbox. As you close a task, capture what you'd tell someone reproducing this from scratch (settings used, gotchas hit, screenshots to take) — that running log becomes the open-source README, so document as you go rather than after the fact.

**Phases**

1. Project setup (PlatformIO + repo scaffolding)
2. GitHub project setup (repo, Issues, Projects board, milestones)
3. Wiring (diagram tools + physical build)
4. TinyML data collection & training (Edge Impulse)
5. Firmware coding & pin configuration
6. Build, flash & upload
7. Testing & validation
8. Documentation & open-source release

## Phase 1 — Project Setup

- [x] Install [PlatformIO](https://platformio.org/) as a VS Code extension (free, open source) — install [VS Code](https://code.visualstudio.com/) first if you don't have it, then add the PlatformIO IDE extension from the marketplace
- [x] Create a new PlatformIO project — **deviation:** created via the PlatformIO "New Project" wizard with `board = esp32s3usbotg`, framework = `arduino`. Note this is technically Espressif's official ESP32-S3-USB-OTG devkit board *definition* (LCD + 4 buttons), not an exact match for the actual hardware (a generic ESP32-S3-N16R8 dev board, confirmed via `esptool flash_id`: ESP32-S3, 16MB quad flash, 8MB quad PSRAM). Kept as-is by choice — pin-mapping mismatches from this will need attention once Phase 3 wiring starts referencing board-specific pin macros.
- [x] Confirm the board builds and uploads a blank sketch before touching sensors — build + upload over USB confirmed working (COM8, native USB on `VID:PID=303A:1001`). Serial output verification was skipped for now (the wizard's default `main.cpp` doesn't call `Serial.begin()`/print anything) — revisit once `main.cpp` actually needs serial output. Note: this board's native USB port required boot to actually route Arduino's `Serial` there in earlier testing; if serial output looks silent later, check whether `-DARDUINO_USB_CDC_ON_BOOT=1` is needed in `build_flags`.
- [x] Set up the repo structure for open-sourcing from day one:
  - `/src` — firmware (`main.cpp`, state machine, drivers)
  - `/include` — headers (pin map, config constants)
  - `/lib` — the Edge Impulse exported library (added in Phase 3)
  - `/docs` — wiring diagrams, photos, this task list exported as the README
  - `/data` — sample IMU captures (small representative samples only — raw training data usually stays out of git; note where the full dataset lives)
  - `/test` — added automatically by the PlatformIO wizard (PlatformIO Unity test runner scaffold); not in the original plan but harmless, left in place
  - `platformio.ini` — board, framework, and library dependencies pinned to versions
- [x] Add a `.gitignore` for PlatformIO (`.pio/`, `.vscode/` build artifacts) — used PlatformIO wizard's auto-generated `.gitignore`
- [x] Initialize the git repo and pick an open-source license (MIT or Apache-2.0 are the common defaults for hobbyist hardware projects) — **chose MIT**, copyright line uses the `msohdy` git identity rather than a legal name (fine for an open-source hobby project); added `LICENSE` and a stub `README.md`
- [x] Create a Telegram bot via [@BotFather](https://t.me/BotFather) and note the bot token + your chat ID — done; token/chat ID currently saved in a local `.env` (now git-ignored) as personal notes. **Note:** `.env` is not read by the firmware — Arduino/PlatformIO can't load it at compile or run time without custom build scripting. Per the PRD, the actual values will move into `include/secrets.h` (a git-ignored C++ header, with a `secrets.h.example` template committed) when Telegram integration is coded in Phase 5.

## Phase 2 — GitHub Project Setup

- [x] Create the GitHub repo (public, since this is going open source) and push the local repo from Phase 1 as the initial commit — created via `gh repo create`: [msohdy/fall-detection-physical-ai-esp32](https://github.com/msohdy/fall-detection-physical-ai-esp32)
- [x] Add repo topics/tags for discoverability (`esp32`, `tinyml`, `edge-impulse`, `fall-detection`, `platformio`)
- [x] Enable GitHub Issues and set up labels that map to this checklist's structure — **deviation:** used `phase-1-setup` through `phase-8-docs` (8 phases, not 7 — matches the actual phase count in this file) plus the repo's default `bug`/`help wanted` labels
- [x] Create a GitHub Project (the built-in Projects board) with columns `Backlog / In Progress / Done` — [Fall Detection Build Roadmap](https://github.com/users/msohdy/projects/3). **Deviation:** one issue per phase (not one per task) was added to the board, each issue linking back to its section of this file for the detailed checklist — keeps the board readable while this file stays the source of truth for task-level detail.
- [x] Set milestones matching the phases, each with its issue attached — 8 milestones created matching the 8 phases in this file
- [ ] Add a `CODEOWNERS` or at least a note in `README.md` on who reviews PRs — **explicitly deferred** to closer to Phase 7/8, per plan
- [ ] Turn on GitHub Discussions — **explicitly deferred** until the project has outside visitors

## Phase 3 — Wiring

**Free/open-source tools to design and document the diagram**

- [ ] [Fritzing](https://fritzing.org/) — open source, breadboard-style diagrams that read well for a general open-source audience; has an ESP32 devkit part in the community library (search Fritzing Parts if it's not bundled)
- [ ] [Wokwi](https://wokwi.com/) — free browser-based simulator with real ESP32-S3 + MPU6050 support; lets you wire and even test firmware logic virtually before touching hardware, and exports a shareable diagram/project link
- [ ] [KiCad](https://www.kicad.org/) — open source, overkill for a breadboard wiring diagram but worth it if you want a proper schematic symbol or plan to make a PCB later
- [ ] Pick one primary tool for the repo (Fritzing or Wokwi cover this project well) and note the choice in `/docs`

**Pin table** (fill in against your specific ESP32-S3 board's silkscreen — pin numbers vary by dev board)

| Signal | MPU6050 | Neopixel | Buzzer |
| --- | --- | --- | --- |
| Power | VCC → 3.3V | VCC → 3.3V/5V per strip spec | VCC → 3.3V |
| Ground | GND → GND | GND → GND | GND → GND |
| Data | SDA → GPIO (I2C SDA) | DIN → GPIO (with \~300–500Ω resistor inline) | signal → GPIO (PWM-capable pin) |
| Clock | SCL → GPIO (I2C SCL) | — | — |

- [ ] Confirm your board's default I2C pins (varies by ESP32-S3 devkit — check the vendor pinout diagram) or plan to set them explicitly in code
- [ ] Confirm the buzzer type (passive vs active) — passive buzzers need a PWM tone, active buzzers just need on/off, and this changes the driver code in Phase 4
- [ ] Note the Neopixel strip's logic voltage — many strips want 5V data with 3.3V logic boards, in which case a level shifter (or the common inline-resistor workaround) may be needed

**Physical build**

- [ ] Breadboard the MPU6050, Neopixel, and buzzer per the diagram — do one component at a time, powering up and sanity-checking (multimeter continuity check) before adding the next
- [ ] Photograph the breadboard from a few angles for the docs — do this now, it's much harder to reconstruct later
- [ ] If building the wearable form factor, plan strain relief on the wires before final assembly (breadboard wiring is fine for bring-up; a wearable needs soldered, secured connections)
- [ ] Add the finished wiring diagram + photos + pin table to `/docs`

## Phase 4 — TinyML Data Collection & Training

**Setup**

- [ ] Create a free [Edge Impulse](https://edgeimpulse.com/) account and a new project
- [ ] Flash a minimal sketch to the ESP32-S3 that reads MPU6050 accel + gyro and prints to serial in the format Edge Impulse's Data Forwarder expects
- [ ] Connect the Data Forwarder (`edge-impulse-data-forwarder` CLI, installed via `npm install -g edge-impulse-cli`) and confirm live samples appear in the Edge Impulse Data Acquisition tab

**Per-class collection** (5 classes: standing, sitting, walking, dizzy, fall)

- [ ] Standing / sitting / walking — several minutes each, varied pace and posture, ideally multiple sessions on different days to avoid overfitting to one specific movement style
- [ ] Dizzy/unsteady — simulate with deliberately irregular gait (staggering, uneven steps); label carefully, expect to need the most retakes since real dizziness can't be reproduced on demand
- [ ] Fall — simulate onto a soft surface (mattress/mat) from forward, backward, and sideways angles to capture different acceleration/orientation signatures; if the device is wearable, capture with it actually worn during the fall, not just held
- [ ] After each recording session, review samples for mislabeled segments (e.g. the few seconds before/after a fall that are actually "standing") and trim or relabel before they pollute training

**Split & balance**

- [ ] Check class balance in Data Acquisition — fall/dizzy will naturally have fewer samples than standing/walking; either collect more of the minority classes or use Edge Impulse's class-balancing options at training time
- [ ] Use Edge Impulse's train/validation/test split (default 80/20 train/test, with validation carved from train) — keep some fall/dizzy samples out of training entirely as a held-out sanity check

**Impulse design & training**

- [ ] Build the impulse: Spectral Analysis processing block + Classification (Neural Network) learning block — Edge Impulse's standard template for accelerometer/gyro continuous motion recognition
- [ ] Tune window size (\~1–2s per the PRD) and window increase/stride to match how quickly a fall actually happens
- [ ] Train, then check the confusion matrix specifically for fall vs standing/sitting misclassification — a missed fall is far costlier than a false alarm here, so bias data collection and thresholds toward catching every fall
- [ ] Target ≥90% accuracy on held-out data per the PRD; if a class lags, that's almost always more/cleaner data for that class rather than a bigger model (these devices have tight memory budgets)

**Export**

- [ ] Deploy as a C++/Arduino library from Edge Impulse (TFLite Micro build)
- [ ] Drop the exported library into `/lib` in the PlatformIO project
- [ ] Note the exact model version/export date in `/docs` — you'll likely retrain more than once, and open-source users will want to know which model a given firmware release ships with

## Phase 5 — Firmware Coding & Pin Configuration

**Pin config**

- [ ] Create `include/pins.h` with named constants for every pin used (I2C SDA/SCL, Neopixel data, buzzer) — matches the pin table from Phase 2, so wiring changes only ever touch one file
- [ ] Add library dependencies to `platformio.ini`: the Edge Impulse export, an MPU6050 driver (e.g. `adafruit/Adafruit MPU6050`), `Adafruit NeoPixel`, and WiFi/HTTPS client (bundled with the ESP32 Arduino core)

**Drivers**

- [ ] MPU6050 read loop — initialize over I2C, pull accel + gyro at the sample rate the Edge Impulse model expects, assemble into the same window format used during data collection (mismatched sample rate/format here is the most common cause of a trained model performing worse on-device than in Edge Impulse's own tests)
- [ ] TFLite Micro inference call using the exported library — feed the window, read back the predicted class
- [ ] Neopixel driver — one function mapping state → color per the PRD's table (standing/sitting/walking = blue/cyan/green, dizzy = amber, alarmed = red pulsing, recovering = amber→green fade)
- [ ] Buzzer driver — `tone()`/`noTone()` or manual PWM depending on passive vs active buzzer (Phase 2); implement `siren_tick()`, `warning_tone_tick()`, `recovery_tone_tick()` as **non-blocking**, timed off `millis()` — not `delay()`, or the IMU sampling loop stalls mid-tone

**State machine**

- [ ] Implement the four states (NORMAL, DIZZY, ALARMED, RECOVERING) and transitions from the PRD's pseudocode/state diagram
- [ ] Implement `CONFIRM_THRESHOLD` and `RECOVER_CONFIRM_THRESHOLD` as sustained-reading counters, not single-frame triggers — a single misread must never silence a real alarm early
- [ ] Implement the ALARMED→RECOVERING→ALARMED relapse path (fall reading during RECOVERING jumps straight back to ALARMED and resets `alert_sent` so a fresh Telegram message goes out)

**Telegram integration**

- [ ] Store the bot token, chat ID, and WiFi credentials in a `secrets.h` that's git-ignored (never commit these) — add `secrets.h.example` template to the repo instead
- [ ] Implement `send_telegram_message()` as an HTTPS POST to the Bot API, called once on ALARMED entry (`alert_sent` flag) and once on RECOVERING→NORMAL resolution
- [ ] Make the WiFi/Telegram call non-blocking or at least short-timeout — per the PRD, a WiFi failure should mean "local alarm still fires, remote notification skipped this cycle," never a stalled main loop
- [ ] Wire it all together in `main.cpp`'s loop per the PRD pseudocode; keep the loop itself thin — sample, classify, update LED, tick the state machine

## Phase 6 — Build, Flash & Upload

- [ ] `pio run` to compile — resolve any missing-library or pin-conflict errors before touching hardware
- [ ] Connect the ESP32-S3 over USB, confirm the correct serial port is detected (`pio device list`)
- [ ] `pio run --target upload` to flash; hold BOOT if your specific board needs manual bootloader entry (some ESP32-S3 boards auto-reset, others don't)
- [ ] Open the serial monitor (`pio device monitor`) and confirm sensor readings, inference output, and WiFi connection logs look sane before disconnecting from USB
- [ ] Do a first untethered test: battery power (if wearable) or wall power (if room device), confirm Neopixel/buzzer behavior matches expectations for a basic "standing still" baseline
- [ ] Note the exact `platformio.ini` board/framework versions that worked in `/docs` — ESP32 Arduino core version mismatches are a common source of "works for me" bug reports from people cloning the repo

## Phase 7 — Testing & Validation

Validate against the PRD's own success criteria:

- [ ] **Classifier accuracy** — re-check ≥90% on held-out data isn't just an Edge Impulse number; run live on-device with samples the model has never seen and log the confusion pattern, especially fall-vs-standing
- [ ] **End-to-end latency** — time from a simulated fall event to the Telegram message landing on a phone; target under \~5 seconds on stable WiFi. Log actual numbers across several trials, not a single run
- [ ] **Zero missed falls** — this is the PRD's explicit priority over false-positive rate. Run repeated fall simulations (forward/backward/sideways, different speeds) and treat any miss as a blocker, not a tuning footnote
- [ ] Test the WiFi-down path deliberately: confirm the local alarm (Neopixel + siren) still fires with WiFi disabled, and that the main loop doesn't stall waiting on the Telegram call
- [ ] Test the relapse path: trigger ALARMED → let it reach RECOVERING → trigger another fall before it reaches NORMAL, confirm it jumps back to ALARMED and sends a fresh alert
- [ ] Test recovery false-clear resistance: brief "standing" misreads during ALARMED should reset the confirm counter, not silence the alarm — deliberately inject a few noisy frames and confirm this holds
- [ ] If wearable: test battery life under continuous sampling + occasional WiFi bursts, and log it (open-source users will ask)
- [ ] Log every test run (setup, result, date) somewhere in `/docs` — this becomes the validation section of the writeup, not something to reconstruct from memory later

## Phase 8 — Documentation & Open-Source Release

- [ ] Write the README from the notes you've collected phase by phase — don't write it from scratch at the end: problem statement, BOM with links, wiring diagram + photos, setup instructions (PlatformIO, Edge Impulse), and a demo GIF/video of a simulated fall triggering the alert
- [ ] Publish the wiring diagram (Fritzing/Wokwi export) and pin table in `/docs`
- [ ] Document the dataset: class list, collection method per class, final sample counts, and either the exported dataset or clear instructions for reproducing it (raw motion data is usually fine to share; note any privacy considerations if it includes identifiable video/audio, which this project doesn't)
- [ ] Include the trained model export (or a link/version note if it's too large for git) so people can flash and test without retraining from zero
- [ ] Document known limitations plainly — false positive rate, what "dizzy" simulation doesn't capture about real dizziness, battery life if wearable, WiFi dependency for the remote alert — this is what separates a genuinely reproducible open-source project from a demo
- [ ] Add a CONTRIBUTING.md if you want outside contributions, and confirm the LICENSE file from Phase 1 is in place
- [ ] Do a final clean-clone test: pull the repo fresh on another machine (or ask someone else to), follow only the README, and see how far they get without asking you anything — fix whatever trips them up
- [ ] Publish: push to GitHub, write a short launch post (blog/LinkedIn/forum) summarizing the problem, the approach, and linking the repo and demo video
