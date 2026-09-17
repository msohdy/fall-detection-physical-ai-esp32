#include <Arduino.h>
#include "neopixel_driver.h"
#include "buzzer_driver.h"

// Phase 5 bring-up test: cycles through every visual/buzzer state
// every 4 seconds so each color and beep pattern can be confirmed by
// eye/ear. Not the final firmware -- see the state machine below.

unsigned long lastSwitchMs = 0;
int stateIndex = 0;
const unsigned long SWITCH_INTERVAL_MS = 4000;

void setup() {
  Serial.begin(115200);
  delay(1000);
  neopixelInit();
  buzzerInit();
  Serial.println("Neopixel + buzzer bring-up test");
}

void loop() {
  neopixelTick();
  buzzerTick();

  if (millis() - lastSwitchMs >= SWITCH_INTERVAL_MS) {
    lastSwitchMs = millis();
    stateIndex = (stateIndex + 1) % 6;
    switch (stateIndex) {
      case 0:
        neopixelSetState(VisualState::Standing);
        buzzerSetState(BuzzerState::Silent);
        Serial.println("Standing (blue, silent)");
        break;
      case 1:
        neopixelSetState(VisualState::Sitting);
        buzzerSetState(BuzzerState::Silent);
        Serial.println("Sitting (cyan, silent)");
        break;
      case 2:
        neopixelSetState(VisualState::Walking);
        buzzerSetState(BuzzerState::Silent);
        Serial.println("Walking (green, silent)");
        break;
      case 3:
        neopixelSetState(VisualState::Dizzy);
        buzzerSetState(BuzzerState::Warning);
        Serial.println("Dizzy (amber, slow intermittent beep)");
        break;
      case 4:
        neopixelSetState(VisualState::Alarmed);
        buzzerSetState(BuzzerState::Siren);
        Serial.println("Alarmed (red pulsing, fast beeping)");
        break;
      case 5:
        neopixelSetState(VisualState::Recovering);
        buzzerSetState(BuzzerState::Recovery);
        Serial.println("Recovering (amber->green fade, triple chirp)");
        break;
    }
  }
}
