#ifndef COLOR_HELPER_H
#define COLOR_HELPER_H

#include "Arduino.h"

// Writes intensity to *LED Color Channels*
void writeToLedChannel(uint8_t channel, uint32_t value);

// Writes RGB Array To LED in one step
void writeRGB(int RGB[]);

#endif /* COLOR_HELPER_H */
