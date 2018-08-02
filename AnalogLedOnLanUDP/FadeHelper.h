#ifndef FADE_HELPER_H
#define FADE_HELPER_H

#include "WiFiUdp.h"

/* Method - Fades between two given color, with a given frame time for each frame
 Returns: True if finished without interruption */
boolean FadeToColorWriter(int currentColor[], int fadeTo[], int frameTime, WiFiUDP* udp);

// Method - Looping through fade colors
void FadeLoop(int loopFrameTime, WiFiUDP* udp);

// Method - Getter for interrupted 
boolean isFadeInterrupted();

#endif /* FADE_HELPER_H */
