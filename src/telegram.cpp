#include "telegram.h"
#include "secrets.h"
#include <Arduino.h>
#include <ctype.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

const unsigned long HTTP_TIMEOUT_MS = 4000;
// Calling WiFi.begin() while a connection attempt is already in
// progress can abort and restart that handshake -- observed to
// stretch a normal ~5-10s WPA2+DHCP connect out to 60+ seconds when
// this was throttled too tight (5s), because it kept interrupting
// itself before the handshake could finish. 15s gives a real attempt
// room to complete before treating it as stuck.
const unsigned long RECONNECT_INTERVAL_MS = 15000;
unsigned long lastReconnectAttempt = 0;

// Owned entirely by the background task below -- the main loop only
// ever sets these true (via telegramSendFallAlert()/telegramSendResolved())
// and never reads them, so there's no real data race to guard against.
volatile bool pendingFallAlert = false;
volatile bool pendingResolved = false;

const unsigned long TASK_POLL_INTERVAL_MS = 500;   // how often the task wakes to check for pending work
const unsigned long SEND_RETRY_INTERVAL_MS = 5000; // throttle between real (WiFi-connected) send attempts

String urlEncode(const char* message) {
  String encoded;
  for (const char* p = message; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += (char)c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

// The actual blocking network call -- only ever invoked from the
// background task below, never from the main loop.
bool sendTelegramMessageBlocking(const char* message) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // No certificate pinning -- a common, accepted simplification for a
  // hobbyist project like this one, at the cost of no protection
  // against a MITM on the local network. See README's Build Log.
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + urlEncode(message);

  http.begin(client, url);
  int httpCode = http.GET();
  http.end();

  return httpCode == 200;
}

// Runs on its own FreeRTOS task/core, entirely separate from the main
// loop's sensor-sampling/classification task. This is what lets a slow
// or retrying HTTPS call happen without ever stalling IMU sampling,
// the buzzer/Neopixel ticks, or classification -- previously, calling
// the blocking send directly from stateMachineUpdate() froze the whole
// main loop for as long as the network call took, which looked
// exactly like the state machine being "stuck."
void telegramTaskLoop(void* /*pvParameters*/) {
  unsigned long lastFallAttemptMs = 0;
  unsigned long lastResolvedAttemptMs = 0;

  for (;;) {
    bool wifiUp = WiFi.status() == WL_CONNECTED;
    unsigned long now = millis();

    if (pendingFallAlert && (!wifiUp || now - lastFallAttemptMs >= SEND_RETRY_INTERVAL_MS)) {
      lastFallAttemptMs = now;
      bool ok = sendTelegramMessageBlocking("Fall detected. Siren active.");
      Serial.printf("[telegram] fall alert send attempt: wifi=%s result=%s\n",
                    wifiUp ? "connected" : "DISCONNECTED", ok ? "OK" : "FAILED");
      if (ok) pendingFallAlert = false;
    }

    if (pendingResolved && (!wifiUp || now - lastResolvedAttemptMs >= SEND_RETRY_INTERVAL_MS)) {
      lastResolvedAttemptMs = now;
      bool ok = sendTelegramMessageBlocking("Resolved: back to normal activity.");
      Serial.printf("[telegram] resolved message send attempt: wifi=%s result=%s\n",
                    wifiUp ? "connected" : "DISCONNECTED", ok ? "OK" : "FAILED");
      if (ok) pendingResolved = false;
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
  }
}

}  // namespace

void telegramInit() {
  WiFi.mode(WIFI_STA);
  // Modem sleep is a known cause of the ESP32 STA link silently
  // dropping after it's been idle/intermittent for a while (works
  // once, then WiFi.status() never reports WL_CONNECTED again without
  // a manual reconnect). Disabling it trades a little extra power draw
  // for a link that actually recovers on its own -- worth it given the
  // "zero missed falls" priority extends to "zero missed alerts."
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // non-blocking; connects in background

  // Pinned to core 0 -- Arduino's setup()/loop() run on core 1 by
  // default, so this keeps Telegram's blocking network calls fully off
  // the core doing sensor sampling and classification. 8KB stack: TLS
  // handshakes (WiFiClientSecure/mbedTLS) need real stack headroom.
  xTaskCreatePinnedToCore(telegramTaskLoop, "telegramTask", 8192, nullptr, 1, nullptr, 0);
}

void telegramTick() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL_MS) {
    return;  // throttle -- don't hammer WiFi.begin() every loop iteration
  }
  lastReconnectAttempt = now;
  Serial.println("[wifi] disconnected -- retrying WiFi.begin()");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // non-blocking; re-associates in background
}

void telegramSendFallAlert() {
  pendingFallAlert = true;
}

void telegramSendResolved() {
  pendingResolved = true;
}
