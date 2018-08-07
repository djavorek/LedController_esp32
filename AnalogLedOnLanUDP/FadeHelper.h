#ifndef FADE_HELPER_H
#define FADE_HELPER_H

#include "WiFiUdp.h"

// Method - For fade loop
void FadeLoop(int loopFrameTime, int isRainbowLoop, WiFiUDP* udp);

// Method - Getter for interrupted 
boolean isFadeInterrupted();

#endif /* FADE_HELPER_H */
