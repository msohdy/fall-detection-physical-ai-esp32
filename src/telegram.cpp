#include "telegram.h"
#include "secrets.h"
#include <Arduino.h>
#include <ctype.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace {

const unsigned long HTTP_TIMEOUT_MS = 4000;

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

}  // namespace

void telegramInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // non-blocking; connects in background
}

bool sendTelegramMessage(const char* message) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;  // skip this cycle -- never block waiting for WiFi
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
