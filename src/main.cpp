#include <Arduino.h>
#include "mpu6050.h"
#include "model_inference.h"
#include "neopixel_driver.h"
#include "buzzer_driver.h"
#include "state_machine.h"
#include "telegram.h"
#include <WiFi.h>

// Main firmware loop. Keeps the loop itself thin per the PRD: sample,
// classify, tick the state machine and drivers.
//
// Classifies on a fresh, non-overlapping window each cycle (not a
// sliding window updated every sample) -- this matches how the model
// was actually validated (independent windows, not overlapping ones)
// and keeps the classification rate low enough that the model's
// measured ~1.2% false-fall-classification rate doesn't compound into
// frequent false alarms. An earlier sliding-window version classified
// on every new sample (50Hz), and the resulting ~50x higher
// classification rate caused spurious ALARMED triggers roughly every
// few seconds even at rest -- see docs/TASKS.md (Phase 5) for the math.

float window[MODEL_INPUT_FEATURES];

void setup() {
  Serial.begin(115200);
  delay(1000);

  mpu6050Init();
  neopixelInit();
  buzzerInit();
  stateMachineInit();
  telegramInit();

  Serial.println("Fall Detection System starting.");
}

void loop() {
  // Tick the animations every ~20ms during window collection (not just
  // once per full window) so the Alarmed pulse / buzzer patterns stay
  // smooth, even though classification itself only happens once per
  // window below.
  for (int i = 0; i < MODEL_WINDOW_SIZE; i++) {
    mpu6050ReadSample(&window[i * MODEL_SENSOR_CHANNELS]);
    neopixelTick();
    buzzerTick();
    telegramTick();
    delay(20);  // ~50Hz, matches training data rate
  }

  int predicted = modelPredict(window);
  Serial.printf("[cycle] predicted=%s wifi=%s\n", modelLabel(predicted),
                WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
  stateMachineUpdate(predicted);

  // One-shot WiFi connect/drop logging -- lets the serial monitor show
  // exactly when the link comes up or disappears, instead of only
  // inferring it indirectly from Telegram send failures.
  static bool wasConnected = false;
  bool isConnected = WiFi.status() == WL_CONNECTED;
  if (isConnected && !wasConnected) {
    Serial.printf("[wifi] connected, IP=%s\n", WiFi.localIP().toString().c_str());
    neopixelFlashWifiConnected();  // purple x3 -- visible without a serial monitor
  } else if (!isConnected && wasConnected) {
    Serial.println("[wifi] connection lost");
  }
  wasConnected = isConnected;
}
