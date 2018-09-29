#ifndef FADE_HELPER_H
#define FADE_HELPER_H

#include "WiFiUdp.h"

// Doing sleep loops
boolean SleepLoop(WiFiUDP* udp);

// Doing fade loops
void FadeLoop(WiFiUDP* udp);

// Returns whether the loop was interrupted by another command or not
boolean isInterrupted();

// Set mode for fade loop
void setFadeMode(int fadeMode);

// Set alpha and speed for fade loop
void setFadeProperties(int fadeAlpha, int fadeSpeed);

// Set total time of fade loop
void setSleepProperties(int sleepTime);

// Set the starting point of the fade
void setFadeStartingPoint(int color[]);

#endif /* FADE_HELPER_H */

