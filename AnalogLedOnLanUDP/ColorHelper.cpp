#include "Arduino.h"

#include "LedDef.h"

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

