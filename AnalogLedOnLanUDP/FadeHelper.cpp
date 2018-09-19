#include <algorithm>
#include "WiFi.h"
#include "WiFiUdp.h"

#include "ColorHelper.h"

boolean throughBlack; //Which mode
boolean toBlack = false; //In through black mode, which step
int interruptedOff[3] = {0, 0, 0};

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

boolean FadeToColorWithFrameTime(int fadeFrom[], int fadeTo[], int frameTime, boolean dynamicFrameTime, WiFiUDP* udp)
{
  int difference[3];
  int actualColor[3] = {fadeFrom[0], fadeFrom[1], fadeFrom[2]};
  
  for (int component = 0; component < 3; component++)
  {
    difference[component] = fadeFrom[component] - fadeTo[component];
    if(difference[component] < 0)
    {
      difference[component] *= (-1);
    }
  }
  
  while (actualColor[0] != fadeTo[0] || actualColor[1] != fadeTo[1] || actualColor[2] != fadeTo[2])
  {
    int actualMaxDifference = 15;
    int normalizedFrameTime = 50;
    
    for (int component = 0; component < 3 ; component++)
    {       
      if(actualColor[component] < fadeTo[component])
      {
        actualColor[component]++;
        difference[component]--;
      }
      else if(actualColor[component] > fadeTo[component])
      {
        actualColor[component]--;
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
      memcpy(from, actualColor, sizeof(actualColor));
      return false;
    }
    
    WriteRGB(actualColor);
    
    if(dynamicFrameTime)
    {
      //Using the third root of difference
      normalizedFrameTime = frameTime / pow(actualMaxDifference, 1.0 / 3);
    }
    else
    {
      normalizedFrameTime = frameTime / 2;
    }
    
    //Defense against too long, too visible frames (not less than about 12fps)
    if(normalizedFrameTime > 83)
    {
      normalizedFrameTime = 83;
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
        if(toBlack)
        {
          isDone = FadeToColorWithFrameTime(from, off, normalizedFrameTime, true, udp);
        }

        if(isDone)
        {
           isDone = FadeToColorWithFrameTime(interruptedOff, to, normalizedFrameTime, false, udp);
           
           if(!isDone)
           {
             toBlack = false;
             memcpy(interruptedOff, from, sizeof(from));
           }
           else
           {
             toBlack = true;
             memcpy(interruptedOff, off, sizeof(off));
           }
        }
      }
      else
      {
        isDone = FadeToColorWithFrameTime(from, to, normalizedFrameTime, true, udp);
      }
      if (!isDone)
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

void setFadeMode(int fadeMode)
{
  if(fadeMode == 1)
  {
    throughBlack = true; 
  }
  else
  {
    throughBlack = false; 
  }
}

void setFadeProperties(double loopAlpha, int loopFrameTime)
{
  //  ALPHA
  //Vanishing the result of old alpha
  from[0] = from[0] / alpha;
  from[1] = from[1] / alpha;
  from[2] = from[2] / alpha;
  
  //Changing the alpha
  if(loopAlpha <= 1)
  {
    if(loopAlpha > 0.10)
    {
       alpha = loopAlpha;
    }
    else
    {
       alpha = 0.10; 
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
  
  //  FRAME TIME
  if(loopFrameTime > 0)
  {
     frameTime = loopFrameTime * ((double)1 / alpha);
  }
  else
  {
     frameTime = 1;
  }
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

