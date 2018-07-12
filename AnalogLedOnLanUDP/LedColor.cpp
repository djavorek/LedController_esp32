// Basics
#include "Arduino.h"

// LED Definitions
#define LED_CHANNEL_RED 0
#define LED_CHANNEL_GREEN 1
#define LED_CHANNEL_BLUE 2


void WriteToLedChannel(uint8_t channel, uint32_t value) 
{
  uint32_t valueMax = 255;
  uint32_t duty = (8191 / valueMax) * _min(value, valueMax);
  ledcWrite(channel, duty);
}

void WriteRGB(int RGB[])
{
  WriteToLedChannel(LED_CHANNEL_RED, RGB[0]);
  WriteToLedChannel(LED_CHANNEL_GREEN, RGB[1]);
  WriteToLedChannel(LED_CHANNEL_BLUE, RGB[2]);
}
