#include <Arduino.h>
#include "FastLED.h"

#define PIN_BUTTON 2
#define PIN_LED 3

#define LED_COUNT 50
#define LED_ACCEPTED_DISTANCE 1
#define LED_MAX_BUTTON_DISTANCE 8
#define BUTTON_DEBOUNCE_MS 20
#define COLOR_GREEN 0x003300
#define COLOR_OFF 0x000000
#define COLOR_RED 0x330000
#define RAINBOW_BRIGHTNESS 50
#define ANIMATION_UPDATE_PERIOD 100
#define LEVEL_ANIMATION_FLASHES 2
#define LEVEL_ANIMATION_ON_TIME (300 / ANIMATION_UPDATE_PERIOD)
#define LEVEL_ANIMATION_OFF_TIME (300 / ANIMATION_UPDATE_PERIOD)

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
  float new_speed = 10 + (current_round)*5;
  duration_per_LED = (uint32_t)(1000000 / new_speed);
}

void setup()
{
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  FastLED.addLeds<WS2812, PIN_LED, RGB>(pixels, LED_COUNT);
  //FastLED.addLeds<NEOPIXEL, PIN_LED>(pixels, LED_COUNT);
  Serial.begin(115200);
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
      if (active_LED < -LED_MAX_BUTTON_DISTANCE)
      {
        //just start another round
        active_LED += LED_COUNT;
      }
      else
      {
        active_LED--;
      }
      last_update_micros = micros();
      for (int i = 0; i < LED_COUNT; i++)
      {
        if (active_LED < 0)
          pixels[i] = (i == LED_COUNT + active_LED) ? COLOR_GREEN : COLOR_OFF;
        else
          pixels[i] = (i == active_LED) ? COLOR_GREEN : COLOR_OFF;
      }
      pixels[0] = COLOR_GREEN;
      FastLED.show();
    }
    if ((button_state_changed && is_pressed) && (abs(active_LED) <= LED_MAX_BUTTON_DISTANCE))
    {
      if (abs(active_LED) <= LED_ACCEPTED_DISTANCE)
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
    //button was pressed, so skip animation
    if (button_state_changed && is_pressed)
    {
      state = Idle;
    }
    else if (millis() - animation_last_update_ms > ANIMATION_UPDATE_PERIOD)
    {
      animation_last_update_ms = millis();
      //animation ended
      if (animation_frame >= LED_COUNT / 2)
      {
        state = Idle;
      }
      else
      {
        for (int i = 0; i < LED_COUNT; i++)
        {
          if (abs(i - LED_COUNT / 2) > animation_frame)
            pixels[i] = COLOR_RED;
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
  { //button was pressed, so skip animation
    if (button_state_changed && is_pressed)
    {
      state = Playing;
    }
    else if (millis() - animation_last_update_ms > ANIMATION_UPDATE_PERIOD)
    {
      animation_last_update_ms = millis();
      int total_duration = LEVEL_ANIMATION_FLASHES * (LEVEL_ANIMATION_ON_TIME + LEVEL_ANIMATION_OFF_TIME);
      if (animation_frame < total_duration)
      {
        uint32_t relative = animation_frame % (LEVEL_ANIMATION_ON_TIME + LEVEL_ANIMATION_OFF_TIME);
        if (relative < LEVEL_ANIMATION_OFF_TIME)
        {
          for (uint32_t i = 0; i < LED_COUNT; i++)
          {
            pixels[i] = COLOR_OFF;
          }
        }
        else
        {
          for (uint32_t i = 0; i < LED_COUNT; i++)
          {
            pixels[i] = (i <= current_round) ? COLOR_GREEN : COLOR_OFF;
          }
        }
        FastLED.show();
      }
      else
      {
        state = Playing;
      }
      animation_frame++;
    }
    break;
  }
  }
}