#include <Arduino.h>
#include "mpu6050.h"
#include "model_inference.h"

// Phase 5 bring-up test: collects one window, runs inference, prints
// the predicted label. Not the final firmware -- confirms the driver +
// inference pipeline works before building the state machine on top of it.

float window[MODEL_INPUT_FEATURES];

void setup() {
  Serial.begin(115200);
  delay(1000);
  mpu6050Init();
  Serial.println("Phase 5 bring-up: MPU6050 + inference pipeline");
}

void loop() {
  for (int i = 0; i < MODEL_WINDOW_SIZE; i++) {
    mpu6050ReadSample(&window[i * MODEL_SENSOR_CHANNELS]);
    delay(20);  // ~50Hz, matches training data rate
  }

  int predicted = modelPredict(window);
  Serial.print("Predicted: ");
  Serial.println(modelLabel(predicted));
}
