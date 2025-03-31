#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "scan_ctl.h"

#define EXT_DISPLAY_PIN 12 // external display pin
#define RGB_LED_PIN 2
#define RGB_LED_COUNT 3

const uint32_t Yellow = Adafruit_NeoPixel::Color(6, 3, 0);
const uint32_t Amber = Adafruit_NeoPixel::Color(6, 2, 0);
const uint32_t Orange = Adafruit_NeoPixel::Color(6, 1, 0);
const uint32_t Red = Adafruit_NeoPixel::Color(6, 0, 0);
const uint32_t Colors[] = {Yellow, Amber, Orange, Red};

Adafruit_NeoPixel rgbLeds(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  pinMode(EXT_DISPLAY_PIN, INPUT_PULLUP);

  initScanCtl();
  rgbLeds.begin();

  Serial.println("Finished setup");
}

int colorIndex = 0;
int colorDelay = 2;
void loop()
{
  rgbLeds.clear();

  bool display = digitalRead(EXT_DISPLAY_PIN);
  setDisplayState(display);

  if (display)
  {
    for (int i = 0; i < RGB_LED_COUNT; i++)
    {
      rgbLeds.setPixelColor(i, Colors[(i + colorIndex) % 3]);
    }
  }
  rgbLeds.show();

  scroll();
  if (--colorDelay <= 0)
  {
    colorDelay = 2;
    colorIndex = (colorIndex + 1) % 3;
  }
  delay(100);
}