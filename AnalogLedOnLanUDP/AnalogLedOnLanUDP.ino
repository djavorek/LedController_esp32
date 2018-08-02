#include "WiFi.h"
#include "WiFiUdp.h"

#include "WifiHelper.h"
#include "NvsHelper.h"
#include "FadeHelper.h"
#include "ColorHelper.h"
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
char* delimiterFound;

// Status
char status[25] = "Off";

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

// Method - Parse the fade properties from UDP Message
void FadeMode(char message[])
{
  int fadeSpeed;
  int fadeFrameTime = 20;
  char * delimittedMsg;

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

  FadeLoop(fadeFrameTime, &udp);
}

// Method - Resets state if something was interrupted
void ResetState()
{
  if (strstr(status, "Fade"))
  {
    FadeLoop(-1, &udp);
  }
}

// Method - Sends a response message to the last client
void AnswerOnUdp(char message[])
{
  udp.beginPacket(udp.remoteIP(), responsePort);
  udp.print(message);
  udp.endPacket();
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
  ConnectToWifi(ssid, password, deviceName);

  // Open UDP
  udp.begin(port);
}

void loop()
{
  // If WiFi gets disconnected, tries to connect again
  if (WiFi.status() != WL_CONNECTED && !WiFi.softAPIP())
  {
    ConnectToWifi(ssid, password, deviceName);
  }

  // Checking for new incoming UDP packet
  if (udp.parsePacket() || isFadeInterrupted())
  {
    // Read the UDP packet into a message variable
    int msgLength = udp.read(udpMessage, 255);
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
    if (strstr(udpMessage, "Status"))
    {
      Serial.println("Status Command!");
      if(strstr(udpMessage, deviceName))
      {
        AnswerOnUdp(status);
        ResetState();
      }
    }

    /*Set Wifi Credentials
    (Credentials:Ssid:Password --> It will connect with(Ssid,Password)*/
    else if (strstr(udpMessage, "Credentials"))
    {
      Serial.println("Credentials Command!");
      SaveWifiCredentials(udpMessage);
    }

    // Fade (Fade --> Normal fade, FadeSpeed:(0-100) --> Customized fade)
    else if (strstr(udpMessage, "Fade"))
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
