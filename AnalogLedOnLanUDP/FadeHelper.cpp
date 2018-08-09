#include <algorithm>
#include "WiFi.h"
#include "WiFiUdp.h"

#include "ColorHelper.h"

boolean throughBlack;
int frameTime;
double alpha = 1;
boolean interrupted;

int off[3] = {0, 0, 0};
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

boolean FadeToColorWithFrameTime(int currentColor[], int fadeTo[], int frameTime, WiFiUDP* udp)
{
  int difference[3];
  
  for (int component = 0; component < 3; component++)
  {
    difference[component] = currentColor[component] - fadeTo[component];
    if(difference[component] < 0)
    {
       difference[component] *= (-1);
    }
  }
  
  while (currentColor[0] != fadeTo[0] || currentColor[1] != fadeTo[1] || currentColor[2] != fadeTo[2])
  {
    int actualMaxDifference = 15;
    
    for (int component = 0; component < 3 ; component++)
    {       
      if(currentColor[component] < fadeTo[component])
      {
        currentColor[component]++;
        difference[component]--;
      }
      else if(currentColor[component] > fadeTo[component])
      {
        currentColor[component]--;
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
      memcpy(from, currentColor, sizeof(currentColor));
      return false;
    }
    
    WriteRGB(currentColor);
    
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
    if(loopAlpha > 0.25)
    {
       alpha = loopAlpha;
    }
    else
    {
       alpha = 0.25; 
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
 }
