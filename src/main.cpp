#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Onboard Neopixel bring-up test: cycles red/green/blue.
// Confirms GPIO48 drives the onboard RGB LED before adding more
// components. Not the final firmware -- see Phase 5.

#define LED_PIN 48
#define LED_COUNT 1

Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(1000);
  pixel.begin();
  pixel.setBrightness(50);
  Serial.println("Neopixel test starting...");
}

void loop() {
  Serial.println("RED");
  pixel.setPixelColor(0, pixel.Color(255, 0, 0));
  pixel.show();
  delay(1000);

  Serial.println("GREEN");
  pixel.setPixelColor(0, pixel.Color(0, 255, 0));
  pixel.show();
  delay(1000);

  Serial.println("BLUE");
  pixel.setPixelColor(0, pixel.Color(0, 0, 255));
  pixel.show();
  delay(1000);
}
