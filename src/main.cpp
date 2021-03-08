#include <Arduino.h>
#include "FastLED.h"

#define PIN_BUTTON A4
#define PIN_LED A5

#define LED_COUNT 24
#define LED_MAX_OVERRUN 0
#define BUTTON_DEBOUNCE_MS 20
#define COLOR_ON 0x005500
#define COLOR_OFF 0x000000
#define COLOR_OVERRUN 0x550000
#define RAINBOW_BRIGHTNESS 100
#define ANIMATION_UPDATE_PERIOD 100

enum GameState
{
  Idle,
  Playing,
  Won,
  Lost
};

CRGB pixels[LED_COUNT];

//LEDs during game
uint32_t last_update_micros;
float circle_duration = 0;
uint32_t duration_per_LED = 0;
int32_t active_LED = 0;

//game variables
uint32_t current_round = 0;
GameState state = Idle;

//key state
bool debounced_key_state = false;
uint32_t debounce_start_ms;

//animations
uint32_t animation_last_update_ms;
int32_t animation_frame;

void update_rotation_speed()
{
  float new_speed = 0.5 + (current_round)*0.2;
  circle_duration = 1 / new_speed;
  duration_per_LED = (uint32_t)(circle_duration * 1000000 / LED_COUNT);
}

void setup()
{
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  FastLED.addLeds<NEOPIXEL, PIN_LED>(pixels, LED_COUNT);
  Serial.begin(115200);
}

void set_single_LED(int32_t index)
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    if (index < 0)
      pixels[i] = (i == LED_COUNT + index) ? COLOR_OVERRUN : COLOR_OFF;
    else
      pixels[i] = (i == index) ? COLOR_ON : COLOR_OFF;
  }
}

void debounce_button(bool *key_changed, bool *key_pressed)
{
  *key_changed = false;
  *key_pressed = debounced_key_state;
  bool raw_state = digitalRead(PIN_BUTTON) == LOW;
  if (raw_state == debounced_key_state)
  {
    debounce_start_ms = millis();
  }
  else if (millis() - debounce_start_ms > BUTTON_DEBOUNCE_MS)
  {
    //changed is only set if the new state was set long enough
    debounced_key_state = raw_state;
    *key_changed = true;
    *key_pressed = debounced_key_state;
  }
}

void lost()
{
  animation_frame = 0;
  state = Lost;
}

void start_game()
{
  current_round = 0;
  active_LED = LED_COUNT - 1;
  update_rotation_speed();
  state = Playing;
}

void won()
{
  current_round++;
  //set active LED one too far
  active_LED = LED_COUNT;
  update_rotation_speed();
  animation_frame = 0;
  state = Won;
}

void loop()
{
  //read button
  bool button_state_changed;
  bool is_pressed;
  debounce_button(&button_state_changed, &is_pressed);
  switch (state)
  {
  case Playing:
  {
    //update LEDs
    if (micros() - last_update_micros > duration_per_LED)
    {
      if (active_LED < -LED_MAX_OVERRUN)
        lost();
      else
        active_LED--;
      last_update_micros = micros();
      set_single_LED(active_LED);
      FastLED.show();
    }
    if (button_state_changed && is_pressed)
    {
      if (abs(active_LED) <= LED_MAX_OVERRUN)
        won();
      else
        lost();
    }
    break;
  }
  case Idle:
  {
    if (millis() - animation_last_update_ms > ANIMATION_UPDATE_PERIOD)
    {
      animation_last_update_ms = millis();
      if (animation_frame >= LED_COUNT)
        animation_frame = 0;
      for (int i = 0; i < LED_COUNT; i++)
      {
        int pos = (i + animation_frame) % LED_COUNT;
        pixels[i].setHSV((uint8_t)((float)pos / (LED_COUNT - 1) * 255), 255, RAINBOW_BRIGHTNESS);
      }
      animation_frame++;
      FastLED.show();
    }
    if (button_state_changed && is_pressed)
    {
      start_game();
    }
    break;
  }
  case Lost:
  {
    if (millis() - animation_last_update_ms > ANIMATION_UPDATE_PERIOD)
    {
      animation_last_update_ms = millis();
      if (animation_frame >= LED_COUNT / 2)
      {
        state = Idle;
      }
      else
      {
        for (int i = 0; i < LED_COUNT; i++)
        {
          if (abs(i - LED_COUNT) < animation_frame)
            pixels[i] = COLOR_OVERRUN;
          else
            pixels[i] = COLOR_OFF;
        }
        animation_frame++;
        FastLED.show();
      }
    }
    break;
  }
  case Won:
  {
    if (millis() - animation_last_update_ms > ANIMATION_UPDATE_PERIOD)
    {
      animation_last_update_ms = millis();
      if (animation_frame >= LED_COUNT / 2)
      {
        state = Playing;
      }
      else
      {
        for (int i = 0; i < LED_COUNT; i++)
        {
          if (abs(i - LED_COUNT) < animation_frame)
            pixels[i] = COLOR_ON;
          else
            pixels[i] = COLOR_OFF;
        }
        animation_frame++;
        FastLED.show();
      }
    }
    break;
  }
  }
}