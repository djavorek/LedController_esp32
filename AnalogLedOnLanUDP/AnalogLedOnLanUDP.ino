// Basics
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// WIFI
#include "WiFi.h"
#include "WiFiUdp.h"

// LED Definitions
#define LED_CHANNEL_RED 0
#define LED_CHANNEL_GREEN 1
#define LED_CHANNEL_BLUE 2

#define LEDC_TIMER_13_BIT 13
#define LEDC_BASE_FREQ 5000

#define LED_PIN_RED 17
#define LED_PIN_GREEN 4
#define LED_PIN_BLUE 16

// HEADERS
#include "ColorHelper.h"
#include "NvsHelper.h"
#include "WifiHelper.h"

//Device Name
char* deviceName = "alpha";

// WiFi Credentials
char ssid[35]      = "";
char password[35]  = "";

// UDP
unsigned int port = 2390;
unsigned int responsePort = 2391;
char udpMessage[255];
WiFiUDP Udp;
char* delimiterFound;

// Status
char status[25] = "Off";

// Observed special commands
char statusCommand[30] = "Status:";
char* credentialsCommand = "Credentials";
char* fadeCommand = "Fade";

/* Fade Settings, used for
start fade again if it was interrupted */
int fadeFrameTime;
int fadeFrom[3] = {0, 0, 0};
int nextColorInFade = 0;
boolean isInterrupted;

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

// Method - Parses the color code from UDP Message and writes it to the LED (e.g.: 255:255:255:255 --> White)
void ColorCodeMode(char message[])
{
  int RGBA[4] = {0, 0, 0, 255};

  // Parses the message
  char* messageSection = messageSection = strtok (message, ":");
  RGBA[0] = atoi(messageSection);

  if (messageSection != NULL)
  {
    messageSection = strtok (NULL, ":");
    RGBA[1] = atoi(messageSection);
  }

  if (messageSection != NULL)
  {
    messageSection = strtok (NULL, ":");
    RGBA[2] = atoi(messageSection);
  }
  
  if (messageSection != NULL)
  {
    messageSection = strtok (NULL, ":");
    RGBA[3] = atoi(messageSection);
  }
  
  // Setting Color Code as Status
  sprintf(status, "%d:%d:%d:%d", RGBA[0], RGBA[1], RGBA[2], RGBA[3]);
  if (strstr(status, "0:0:0:"))
  {
    strcpy(status, "Off");
  }

  AnswerOnUdp(status);

  float alpha = (float)RGBA[3] / (float)255;
  RGBA[0] = RGBA[0] * alpha;
  RGBA[1] = RGBA[1] * alpha;
  RGBA[2] = RGBA[2] * alpha;

  WriteRGB(RGBA);
}

/* Method - Fades between two given color, with a given delay speed between each frame
 Returns: True if finished without interruption */
boolean FadeToColorWriter(int currentColor[], int fadeTo[], int frameTime)
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
    if (Udp.parsePacket() != 0)
    {
      memcpy(fadeFrom, currentColor, sizeof(currentColor));
      return false;
    }

    WriteRGB(currentColor);
    delay(frameTime);
  }
  
  return true;
}

// Method - Looping through fade colors
void FadeLoop(int startColor[3], int loopColors[][3], int nextColor)
{
  int off[3] = {0, 0, 0};

  Serial.println();
  Serial.print("In fade loop, delay: ");
  Serial.println(fadeFrameTime);

  // Fading Process
  do
  {
    // It is used for finishing interrupted fade loops, then the normal loop can start
    if (nextColor != 0)
    {
      for (int color = nextColor; color < 8; color++)
      {
        Serial.println("New Fade Color");
        boolean isDone = FadeToColorWriter(startColor, loopColors[color], fadeFrameTime);
        if (isDone != true)
        {
          isInterrupted = true;
          nextColorInFade = color;
          WriteRGB(off);
          return;
        }
        else
        {
          memcpy(startColor, loopColors[color], sizeof(startColor));
        }
      }

      nextColor = 0; // To skip this section later on
    }

    for (int color = 0; color < 8; color++)
    {
      Serial.println("New Fade Color");
      boolean isDone = FadeToColorWriter(startColor, loopColors[color], fadeFrameTime);
      if (isDone != true)
      {
        isInterrupted = true;
        nextColorInFade = color;
        WriteRGB(off);
        return;
      }
      else
      {
        memcpy(startColor, loopColors[color], sizeof(startColor));
      }
    }
  } while (WiFi.status() == WL_CONNECTED);
}

// Method - Parse the fade properties from UDP Message
void FadeMode(char message[])
{
  int fadeSpeed;
  char * delimittedMsg;

  fadeFrameTime = 20;

  // Checking the message to know, its speed customized, or basic
  delimittedMsg = strtok (message, ":");
  if (!strcmp(delimittedMsg, "FadeSpeed"))
  {
    delimittedMsg = strtok (NULL, ":");
    fadeSpeed = atoi(delimittedMsg);

    /* We actually use minor delays between frames,
    but for users the speed is more convenient */
    fadeFrameTime = (101 - fadeSpeed) / 4;
  }

  // Updating status and sending it back as a response
  sprintf(status, "Fade:%d", fadeSpeed);
  AnswerOnUdp(status);

  FadeLoop(fadeFrom, fadePalette, nextColorInFade);
}

// Method - Resets state if something was interrupted
void ResetState()
{
  if (strstr(status, "Fade"))
  {
    FadeLoop(fadeFrom, fadePalette, nextColorInFade);
  }
}

// Method - Sends a response message to the last client
void AnswerOnUdp(char message[])
{
  Udp.beginPacket(Udp.remoteIP(), responsePort);
  Udp.print(message);
  Udp.endPacket();
}

void setup()
{
  // Initialize LED Color Channels (R,G,B)
  ledcSetup(LED_CHANNEL_RED, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_RED, LED_CHANNEL_RED);

  ledcSetup(LED_CHANNEL_GREEN, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_GREEN, LED_CHANNEL_GREEN);

  ledcSetup(LED_CHANNEL_BLUE, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_BLUE, LED_CHANNEL_BLUE);
  
  //Concatting device name to status command
  strncat(statusCommand, deviceName, sizeof(statusCommand) - strlen(statusCommand));

  // Open Serial Printing
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();

  // Load the credentials from NVS
  LoadWifiCredentials(ssid, sizeof(ssid), password, sizeof(password));

  // Connect to WiFi
  ConnectToWifi(ssid, password, deviceName);

  // Open UDP
  Udp.begin(port);
}

void loop()
{
  // If WiFi gets disconnected, tries to connect again
  if (WiFi.status() != WL_CONNECTED && !WiFi.softAPIP())
  {
    ConnectToWifi(ssid, password, deviceName);
  }

  // Checking for new incoming UDP packet
  if (Udp.parsePacket() || isInterrupted == true)
  {
    isInterrupted = false;

    // Read the UDP packet into a message variable
    int msgLength = Udp.read(udpMessage, 255);
    if (msgLength > 0)
    {
      udpMessage[msgLength] = 0;
    }

    Serial.println();
    Serial.println("UDP Packet Received: ");
    Serial.println(udpMessage);
    Serial.println();
    
    /*Answers the current status
    (Status --> Fade, Status --> 255:255:255:255, Status --> Off)*/
    if (strstr(udpMessage, statusCommand))
    {
      Serial.println("Status Command!");
      AnswerOnUdp(status);
      ResetState();
    }

    /*Set Wifi Credentials
    (Credentials:Ssid:Password --> It will connect with(Ssid,Password)*/
    else if (strstr(udpMessage, credentialsCommand))
    {
      Serial.println("Credentials Command!");
      SaveWifiCredentials(udpMessage);
    }

    // Fade (Fade --> Normal fade, FadeSpeed:(0-100) --> Customized fade)
    else if (strstr(udpMessage, fadeCommand))
    {
      Serial.println("Fade Command!");
      FadeMode(udpMessage);
    }

    // RGBA Color Code (255:255:255:255 --> White)
    else if (strchr(udpMessage, ':'))
    {
      Serial.println("Color Code Command!");
      ColorCodeMode(udpMessage);
    }
  }
}
