#include "WiFi.h"
#include "WiFiUdp.h"

#include "ColorHelper.h"

int frameTime;
boolean interrupted;
int isRainbow = 0;

//Fade (with palette)
int from[3] = {0, 0, 0};
int nextColorFromPalette = 0;

//Rainbow
int savedProgress = 0;

int fadePalette[10][3] =
{
  {255, 70, 0}, // RED
  {50, 50, 255}, // BLUE
  {255, 120, 0}, // ORANGE
  {0, 255, 150}, // GREEN
  {255, 0, 185}, // PURPLE
  {255, 243, 18}, // YELLOW
  {0, 255, 255}, // LIGHT BLUE
  {80, 0, 240} // DARK BLUE
};

/* Method - Fades between two given color, with a given delay speed between each frame
 Returns: True if finished without interruption */
boolean FadeToColorWriter(int currentColor[], int fadeTo[], int frameTime, WiFiUDP* udp)
{
  while (currentColor[0] != fadeTo[0] || currentColor[1] != fadeTo[1] || currentColor[2] != fadeTo[2])
  {
    for (int component = 0; component < 3 ; component++)
    {
      if (currentColor[component] != fadeTo[component])
      {        
        if(currentColor[component] < fadeTo[component]) currentColor[component]++;
        else currentColor[component]--; 
      }
    }

    // If UDP Received, stop fading
    if (udp->parsePacket() != 0)
    {
      memcpy(from, currentColor, sizeof(currentColor));
      return false;
    }

    WriteRGB(currentColor);
    delay(frameTime);
  }
  return true;
}

// Method - Looping through fade colors
void FadeLoop(int loopFrameTime, int isRainbowLoop, WiFiUDP* udp)
{
  interrupted = false;
  
  if (loopFrameTime != -1)
  {
    frameTime = loopFrameTime;
  }
  
  if (isRainbowLoop != -1)
  {
     isRainbow = isRainbowLoop;
  }
  
  int off[3] = {0, 0, 0};

  Serial.println();
  Serial.print("Fade, with frame time: ");
  Serial.println(frameTime);

  do
  {
    if (isRainbow)
    {
      Serial.println("Rainbow Loop");
      for (int progress=savedProgress; progress < 360; progress++)
      {
        Serial.println("Rainbow progress..");
        float x = float(progress);
        int r = int(127*(sin(x/180*PI)+1));
        int g = int(127*(sin(x/180*PI+3/2*PI)+1));
        int b = int(127*(sin(x/180*PI+0.5*PI)+1));
        
        int color[3] = {r,g,b};
        WriteRGB(color);
        delay(frameTime);
        
        // If UDP Received, stop fading
        if (udp->parsePacket() != 0)
        {
        savedProgress = progress++;
        interrupted = true;
        WriteRGB(off);
        return;
        }
      }
      savedProgress = 0;
    }
    else
    {
      Serial.println("Fade Loop");
      for (int color = nextColorFromPalette; color < 8; color++)
      {
        Serial.println("New Fade Color");
        boolean isDone = FadeToColorWriter(from, fadePalette[color], frameTime, udp);
        if (isDone != true)
        {
          nextColorFromPalette = color;
          interrupted = true;
          WriteRGB(off);
          return;
        }
        else
        {
          memcpy(from, fadePalette[color], sizeof(fadePalette[color]));
        }
      }
      nextColorFromPalette = 0;
    }
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
