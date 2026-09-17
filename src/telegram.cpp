#include "telegram.h"
#include <Arduino.h>

// Placeholder -- real HTTPS POST to the Telegram Bot API is implemented
// in the next Phase 5 task group (Telegram integration). For now this
// lets the state machine be built and tested independently.
void sendTelegramMessage(const char* message) {
  Serial.print("[telegram stub] ");
  Serial.println(message);
}
