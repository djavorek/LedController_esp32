#include <algorithm>
#include "WiFi.h"
#include "WiFiUdp.h"

#include "ColorHelper.h"

boolean throughBlack;
int frameTime;
double alpha = 1;
boolean interrupted;

int off[3] = {0, 0, 0};
int sleepState[3] = {0, 0, 0};
int from[3] = {0, 0, 0};
int to[3];
int nextColorFromPalette = 0;
int fadePalette[6][3] =
{
  {255, 0, 0},
  {235, 255, 0},
  {0, 0, 255},
  {255, 0, 235},
  {0, 255, 0},
  {0, 235, 255}
};

boolean FadeToColorWithFrameTime(int from[], int fadeTo[], int frameTime, WiFiUDP* udp)
{
  int difference[3];
  
  for (int component = 0; component < 3; component++)
  {
    difference[component] = from[component] - fadeTo[component];
    if(difference[component] < 0)
    {
      difference[component] *= (-1);
    }
  }
  
  while (from[0] != fadeTo[0] || from[1] != fadeTo[1] || from[2] != fadeTo[2])
  {
    int actualMaxDifference = 15;
    
    for (int component = 0; component < 3 ; component++)
    {       
      if(from[component] < fadeTo[component])
      {
        from[component]++;
        difference[component]--;
      }
      else if(from[component] > fadeTo[component])
      {
        from[component]--;
        difference[component]--;
      }
        
      if(difference[component] > actualMaxDifference)
      {
        actualMaxDifference = difference[component]; 
      }
    }

    // If UDP Received, stop fading
    if (udp->parsePacket() != 0)
    {
      memcpy(from, from, sizeof(from));
      return false;
    }
    
    WriteRGB(from);
    
    //Using the third root of difference for the optimal result
    int normalizedFrameTime = frameTime / pow(actualMaxDifference, 1.0 / 3);
    
    //Defense against too long, too visible frames (not less than about 15fps)
    if(normalizedFrameTime > 70)
    {
      normalizedFrameTime = 70;
    }

    delay(normalizedFrameTime);
  }
  return true;
}

boolean SleepLoop(WiFiUDP* udp)
{
  int udpCheckTime = 200;
  int maxDifference = 1;
  int fixedFrom[3] = {sleepState[0], sleepState[1], sleepState[2]};
  float alpha;
  
  interrupted = false;
  
  for (int component = 0; component < 3; component++)
  { 
    if(from[component] > maxDifference)
    {
      maxDifference = from[component]; 
    }
  }
  int fixedMaxDifference = maxDifference;
  int sleepFrameTime = frameTime / maxDifference;
  div_t delayCyclesNeeded = div(sleepFrameTime, udpCheckTime);
  
  WriteRGB(from);
  delay(delayCyclesNeeded.rem);
  frameTime -= delayCyclesNeeded.rem;
  
  while (from[0] != 0 || from[1] != 0 || from[2] != 0)
  {
    alpha = (float)maxDifference / (float)fixedMaxDifference;
    maxDifference--;
    from[0] = fixedFrom[0] * alpha;
    from[1] = fixedFrom[1] * alpha;
    from[2] = fixedFrom[2] * alpha;
    
    WriteRGB(from);
    
    for(int i = 0; i < delayCyclesNeeded.quot; i++)
    {
      delay(udpCheckTime);
      frameTime -= udpCheckTime;
      
      // If UDP Received, stop fading
      if (udp->parsePacket() != 0)
      {
        interrupted = true;
        return false;
      }
    }
  }
  return true;
}

// Method - Looping through fade colors
void FadeLoop(WiFiUDP* udp)
{
  interrupted = false;
  boolean isDone = true;
  int normalizedFrameTime = (int)(frameTime * (255 / (255 * alpha)));
  
  do
  {
    for (int color = nextColorFromPalette; color < 6; color++)
    {
      to[0] = fadePalette[color][0] * alpha;
      to[1] = fadePalette[color][1] * alpha;
      to[2] = fadePalette[color][2] * alpha;
      
      if(throughBlack)
      {
        isDone = FadeToColorWithFrameTime(from, off, normalizedFrameTime, udp);
      }
      else
      {
        isDone = FadeToColorWithFrameTime(from, to, normalizedFrameTime, udp);
      }
      if (isDone != true)
      {
        nextColorFromPalette = color;
        interrupted = true;
        WriteRGB(off);
        return;
      }
      else
      {
        from[0] = fadePalette[color][0] * alpha;
        from[1] = fadePalette[color][1] * alpha;
        from[2] = fadePalette[color][2] * alpha;
      }
    }
    nextColorFromPalette = 0;
  } while (WiFi.status() == WL_CONNECTED);
}

boolean isFadeInterrupted()
{
  if(interrupted)
  {
    interrupted = false;
    return true; 
  }
  return false; 
}

void setLoopMode(int loopMode)
{
  if(loopMode == 1)
  {
    throughBlack = true; 
  }
  else
  {
    throughBlack = false; 
  }
}

void setLoopFrameTime(int loopFrameTime)
{
  if(loopFrameTime > 0)
  {
     frameTime = loopFrameTime; 
  }
  else
  {
    frameTime = 1;
  }
}

void setLoopAlpha(double loopAlpha)
{
  //Vanishing the result of old alpha
  from[0] = from[0] / alpha;
  from[1] = from[1] / alpha;
  from[2] = from[2] / alpha;
  
  //Changing the alpha
  if(loopAlpha <= 1)
  {
    if(loopAlpha > 0.15)
    {
       alpha = loopAlpha;
    }
    else
    {
       alpha = 0.15; 
    }
  }
  else
  {
    alpha = 1;
  }
  
  //Using the new alpha
  from[0] = from[0] * alpha;
  from[1] = from[1] * alpha;
  from[2] = from[2] * alpha;
}

void setFadeStartingPoint(int color[])
{
  from[0] = color[0];
  from[1] = color[1];
  from[2] = color[2];
  sleepState[0] = color[0];
  sleepState[1] = color[1];
  sleepState[2] = color[2];
}

