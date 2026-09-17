#ifndef NEOPIXEL_DRIVER_H
#define NEOPIXEL_DRIVER_H

// Onboard single WS2812 LED driver. Color-per-state mapping from the
// PRD: standing/sitting/walking = blue/cyan/green, dizzy = amber,
// alarmed = red pulsing, recovering = amber->green fade.

enum class VisualState {
  Standing,
  Sitting,
  Walking,
  Dizzy,
  Alarmed,
  Recovering,
};

void neopixelInit();

// Call once whenever the visual state changes (not every loop) -- sets
// the steady color immediately and resets animation timing for
// Alarmed/Recovering.
void neopixelSetState(VisualState state);

// Call every loop iteration. No-op for steady states; drives the
// pulsing/fading animation for Alarmed/Recovering, timed off millis()
// (non-blocking).
void neopixelTick();

// One-shot: flashes purple 3 times (non-blocking, ticked via
// neopixelTick() like everything else), then resumes whatever the
// current VisualState should be showing. Call whenever WiFi connects,
// so field testing without a serial monitor has a visible signal.
void neopixelFlashWifiConnected();

#endif  // NEOPIXEL_DRIVER_H
