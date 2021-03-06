#include <Arduino.h>
#include "FastLED.h"

#define PIN_BUTTON A4
#define PIN_LED A5
#define LED_COUNT 24

CRGB pixels[LED_COUNT];
CRGB color_on = 0x005500;
CRGB color_off = 0x000000;
CRGB color_penalty = 0x550000;

uint32_t debounce_time_ms = 20;

uint32_t last_update_micros;
float circle_duration = 0;
uint32_t duration_per_LED = 0;
uint8_t active_LED = 0;

bool was_pressed = false;
uint32_t debounce_start_ms;

void set_circle_duration(float new_duration)
{
  circle_duration = new_duration;
  duration_per_LED = (uint32_t)(circle_duration * 1000000 / LED_COUNT);
}

void setup()
{
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  FastLED.addLeds<NEOPIXEL, PIN_LED>(pixels, LED_COUNT);
  set_circle_duration(2);
  active_LED = LED_COUNT - 1;
}

void set_single_LED(int index)
{
  for (int i = 0; i < LED_COUNT; i++)
    pixels[i] = (i == index) ? color_on : color_off;
}

void loop()
{
  if (micros() - last_update_micros > duration_per_LED)
  {
    if (active_LED > 0)
      active_LED--;
    last_update_micros = micros();
    set_single_LED(active_LED);
    FastLED.show();
  }
  if (digitalRead(PIN_BUTTON) == LOW)
  {
    if (!was_pressed)
    {
      was_pressed = true;
      debounce_start_ms = millis();
      last_update_micros = micros();
      active_LED = LED_COUNT - 1;
      set_single_LED(active_LED);
      was_pressed = true;
      FastLED.show();
    }
  }
  else
  {
    if (millis() - debounce_start_ms > debounce_time_ms)
      was_pressed = false;
  }
}