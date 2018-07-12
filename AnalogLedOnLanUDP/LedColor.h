#ifndef LED_COLOR_H
#define LED_COLOR_H

//Method - Writes intensity to *LED Color Channel*
void WriteToLedChannel(uint8_t channel, uint32_t value);

// Method - Writes RGB Array To LED in one step
void WriteRGB(int RGB[]);

#endif /* LED_COLOR_H */
