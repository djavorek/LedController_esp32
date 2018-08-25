#ifndef FADE_HELPER_H
#define FADE_HELPER_H

#include "WiFiUdp.h"

// Doing sleep loops
boolean SleepLoop(WiFiUDP* udp);

// Doing fade loops
void FadeLoop(WiFiUDP* udp);

// Getter for interrupted 
boolean isFadeInterrupted();

// Set mode for fade loop
void setLoopMode(int loopMode);

// Set frame time for fade loop
void setLoopFrameTime(int loopFrameTime);

// Set alpha for fade loop
void setLoopAlpha(double loopAlpha);

// Set the starting point of the fade
void setFadeStartingPoint(int color[]);

#endif /* FADE_HELPER_H */

