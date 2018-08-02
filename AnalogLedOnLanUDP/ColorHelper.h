#ifndef COLOR_HELPER_H
#define COLOR_HELPER_H

#include "Arduino.h"

//Method - Writes intensity to *LED Color Channels*
void WriteToLedChannel(uint8_t channel, uint32_t value);

// Method - Writes RGB Array To LED in one step
void WriteRGB(int RGB[]);

#endif /* COLOR_HELPER_H */
