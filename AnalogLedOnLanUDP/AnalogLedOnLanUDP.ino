#include "WiFi.h"
#include "WiFiUdp.h"

#include "WifiHelper.h"
#include "NvsHelper.h"
#include "ColorHelper.h"
#include "FadeHelper.h"
#include "LedDef.h"

//Device Name
char* deviceName = "alpha";

// WiFi Credentials
char ssid[35]      = "";
char password[35]  = "";

// UDP
unsigned int port = 2390;
unsigned int responsePort = 2391;
char udpMessage[255];
WiFiUDP udp;

// Status
char status[25] = "Off";

// Parses the color code from UDP Message and writes it to the LED (e.g.: 255:255:255:255 --> White)
void ColorCodeMode(char message[])
{
  int RGBA[4] = {0, 0, 0, 255};

  // Parses the message
  char* messageSection = messageSection = strtok (message, ":");
  if (messageSection != NULL)
  {
    RGBA[0] = atoi(messageSection);
  }

  messageSection = strtok (NULL, ":");
  if (messageSection != NULL)
  {
    RGBA[1] = atoi(messageSection);
  }

  messageSection = strtok (NULL, ":");
  if (messageSection != NULL)
  {
    RGBA[2] = atoi(messageSection);
  }
  
  messageSection = strtok (NULL, ":");
  if (messageSection != NULL)
  {
    RGBA[3] = atoi(messageSection);
  }

  // Setting Color Code as Status
  sprintf(status, "%d:%d:%d:%d", RGBA[0], RGBA[1], RGBA[2], RGBA[3]);
  if (strstr(status, "0:0:0:"))
  {
    strcpy(status, "Off");
  }
  
  AnswerOnUdp(status);

  //Intentionally written after status set, so the status is more meaningful (not showing black color)
  float alpha = (float)RGBA[3] / (float)255;
  RGBA[0] = RGBA[0] * alpha;
  RGBA[1] = RGBA[1] * alpha;
  RGBA[2] = RGBA[2] * alpha;
  
  //If the next command is fade, it will start from this color
  setFadeStartingPoint(RGBA);
  
  WriteRGB(RGBA);
}

// Parses the fade properties from UDP Message
void FadeMode(char message[])
{
  char * fadeProperty;
  int fadeMode = 0;
  int fadeSpeed = 128;
  int fadeFrameTime;
  int fadeAlpha = 255;

  //Fade String
  fadeProperty = strtok (message, ":");
  
  //Mode
  fadeProperty = strtok (NULL, ":");
  if (fadeProperty != NULL)
  {
    fadeMode = atoi(fadeProperty);
  }
  
  //Speed
  fadeProperty = strtok (NULL, ":");
  if (fadeProperty != NULL)
  {
    fadeSpeed = atoi(fadeProperty);
    if(fadeSpeed > 255)
    {
      fadeSpeed = 255; 
    }
    else if(fadeSpeed < 1)
    {
      fadeSpeed = 1; 
    }
  }
  
  //Alpha
  fadeProperty = strtok (NULL, ":");
  if (fadeProperty != NULL)
  {
    fadeAlpha = atoi(fadeProperty);
    if(fadeAlpha > 255)
    {
      fadeAlpha = 255; 
    }
    else if(fadeAlpha < 1)
    {
      fadeAlpha = 1; 
    }
  }

  // Updating status and sending it back as a response
  sprintf(status, "Fade:%d:%d:%d", fadeMode, fadeSpeed, fadeAlpha);
  AnswerOnUdp(status);
   
  setFadeMode(fadeMode);
  setFadeProperties(fadeAlpha, fadeSpeed);
  FadeLoop(&udp);
}

// Parses the sleep properties from UDP Message
void SleepMode(char message[])
{
  char * sleepProperty;
  int hours = 0;
  int minutes = 1;
  int totalTime = 0;

  // Sleep String
  sleepProperty = strtok (message, ":");
  
  // Hours
  sleepProperty = strtok (NULL, ":");
  if (sleepProperty != NULL)
  {
    hours = atoi(sleepProperty);
  }
  
  // Minutes
  sleepProperty = strtok (NULL, ":");
  if (sleepProperty != NULL)
  {
    minutes = atoi(sleepProperty);
  }
  
  sprintf(status, "Sleep:%d:%d", hours, minutes);
  AnswerOnUdp(status);
  
  minutes += hours * 60;
  totalTime = minutes * 60000; // 1min = 60000ms
  
  setSleepProperties(totalTime);
  boolean done = SleepLoop(&udp);
  if(done)
  {
    sprintf(status, "Off");
  }
}

// Resets state, if a looped state was interrupted
void ResetState(char currentState[])
{
  if (strstr(currentState, "Fade"))
  {
    FadeLoop(&udp);
  }
  else if (strstr(currentState, "Sleep"))
  {
    SleepLoop(&udp);
  }
  else if (strstr(currentState, ":"))
  {
    ColorCodeMode(currentState);
  }
}

// Sends a response message to the last client, it communicated with
void AnswerOnUdp(char message[])
{
  udp.beginPacket(udp.remoteIP(), responsePort);
  udp.print(message);
  udp.endPacket();
  
  Serial.print("Status sent: ");
  Serial.println(message);
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
  
  // Open Serial Printing
  Serial.begin(115200);
  Serial.println();

  // Load the credentials from NVS
  LoadWifiCredentials(ssid, sizeof(ssid), password, sizeof(password));

  // Connect to WiFi
  ConnectToWifi(ssid, password);
  
  // If WiFi cannot be connected, create WiFi HotSpot
  if(WiFi.status() != WL_CONNECTED)
  {
    HostSoftAP(deviceName);
  }

  // Open UDP
  udp.begin(port);
}

void loop()
{
  // If WiFi gets disconnected, tries to reconnect
  if (WiFi.status() != WL_CONNECTED && !isHotspotHosted())
  {
    ConnectToWifi(ssid, password);
    
    // If it gets reconnected for the first try, state will be restored
    // Otherwise led wil be turned off permanently
    if (WiFi.status() != WL_CONNECTED)
    {
      sprintf(status, "Off");
    }
    else
    {
      ResetState(status);
    }
  }

  // Checking for new incoming UDP packet
  if (udp.parsePacket() || isInterrupted())
  {
    // Read the UDP packet into a message variable
    int msgLength = udp.read(udpMessage, 128);
    if (msgLength > 0)
    {
      udpMessage[msgLength] = 0;
    }

    Serial.println();
    Serial.println("UDP Packet Received: ");
    Serial.println(udpMessage);
    Serial.println();
    
    /*Answers the current status
    (Status:deviceName --> Fade, Status:deviceName --> 255:255:255:255, Status:deviceName --> Off)*/
    if (strstr(udpMessage, "Status"))
    {
      Serial.println("Status Command!");
      if(strstr(udpMessage, deviceName))
      {
        AnswerOnUdp(status);
        ResetState(status);
      }
    }

    /*Set Wifi Credentials
    (Credentials:ssid:password --> It will connect with(Ssid,Password)*/
    else if (strstr(udpMessage, "Credentials"))
    {
      Serial.println("Credentials Command!");
      SaveWifiCredentials(udpMessage);
    }

    // Fade (Fade:mode:speed:alpha --> Starts Fade from the saved state, with given mode, speed, alpha)
    else if (strstr(udpMessage, "Fade"))
    {
      Serial.println("Fade Command!");
      FadeMode(udpMessage);
    }

    // Sleep
    else if (strstr(udpMessage, "Sleep"))
    {
      Serial.println("Sleep Command!");
      SleepMode(udpMessage);
    }
    
    // RGBA Color Code (255:255:255:255 --> White)
    else if (strchr(udpMessage, ':'))
    {
      Serial.println("Color Code Command!");
      ColorCodeMode(udpMessage);
    }
  }
}
