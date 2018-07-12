// Basics
#include <algorithm>
#include <string.h>

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
#include "LedColor.h"
#include "NvsCredential.h"
#include "WifiHelper.h"

// WiFi Credentials
char ssid[]      = "";
char password[]  = "";

// UDP
unsigned int port = 2390;
unsigned int responsePort = 2391;
char udpMessage[255];
WiFiUDP Udp;
boolean isInterrupted;
char* delimiterFound;

// Status
char status[25] = "Off";

/* Fade Settings, most of these used for
start fade again if it was interrupted */
int delayTime;
int startAt[3] = {0, 0, 0};
int nextColorInFade = 0;

int defaultFade[10][3] = // Valid Fade Settings
 {
  {255, 255, 255}, //White
  {255, 2, 0}, // RED
  {0, 255, 0}, // GREEN
  {255, 0, 10}, // PURPLE
  {255, 243, 18}, // YELLOW
  {0, 50, 255}, // LIGHT BLUE
  {255, 165, 0}, // ORANGE
  {192, 255, 62}, // LIGHT GREEN
  {0, 0, 153} // DARK BLUE
 };
 

// Method - Parses the color code from UDP Message and writes it to the LED (e.g.: 255:255:255 --> White)
void ColorCodeMode(char message[])
{
  char * colorIntensity;
  int RGB[3] = {0,0,0};
  
  // Parses the message
  colorIntensity = strtok (message, ":");
  RGB[0] = atoi(colorIntensity);

  if (colorIntensity != NULL)
  {
    colorIntensity = strtok (NULL, ":");
    RGB[1] = atoi(colorIntensity);
  }

  if (colorIntensity != NULL)
  {
    colorIntensity = strtok (NULL, ":");
    RGB[2] = atoi(colorIntensity);
  }
  
  // Setting Color Code as Status
  sprintf(status, "%d:%d:%d", RGB[0], RGB[1], RGB[2]);
  if(strcmp(status, "0:0:0") == 0)
  {
    strcpy(status, "Off");
  }
  
  AnswerOnUdp(status);

  WriteRGB(RGB);
}

/* Method - Fades between two given color, with a given delay speed between each frame
 Returns: True if finished without interruption */
boolean FadeToColorWriter(int currentState[], int fadeTo[], int delTime)
{
  int colorDifferences[3];
  int colorDifferencesNotNegative[3];
  div_t increasement[3];
  int howManySteps[3];

  // Calculates the differences between each components of the two color
  for (int component = 0; component < 3; component++)
  {
    colorDifferences[component] = fadeTo[component] - currentState[component];
    if (colorDifferences[component] < 0)
    {
      colorDifferencesNotNegative[component] = colorDifferences[component] * -1;
    }
    else
    {
      colorDifferencesNotNegative[component] = colorDifferences[component];
    }
  }

  // Helps to reduce the number of fade steps
  for (int component = 0; component < 3 ; component++)
  {
    howManySteps[component] = colorDifferencesNotNegative[component] / 2;
    if (howManySteps[component] == 0) {
      howManySteps[component] = 1;
    }
    
    increasement[component] = div(colorDifferences[component], howManySteps[component]);
    currentState[component] = currentState[component] + increasement[component].rem;
  }

  /* "Step before start", - to reach the perfect color as result,
   it writes the remenent color differences first to be nearly invisible, still simple*/
  WriteRGB(currentState);
  delay(delTime);

  // Cycles through the steps, 'till we reach the needed color
  while (currentState[0] != fadeTo[0] || currentState[1] != fadeTo[1] || currentState[2] != fadeTo[2])
  {
    for (int component = 0; component < 3 ; component++)
    {
      if (currentState[component] != fadeTo[component])
      {
        currentState[component] += increasement[component].quot;
      }
    }
    
    // If UDP Received, stop fading
    if (Udp.parsePacket() != 0)
    {
      memcpy(startAt, currentState, sizeof(currentState));
      return false;
    }

    WriteRGB(currentState);
    Serial.println("Changing color..");
    delay(delTime);
  }
  
  return true;
}

// Method - Looping through fade colors
void FadeLoop(int startColor[3], int loopColors[][3], int nextColor)
{
  int off[3] = {0, 0, 0};

  Serial.println();
  Serial.print("In fade loop, delay: ");
  Serial.println(delayTime);
    
  // Fading Process
  do
  {
    // It is used for finishing interrupted fade loops, then the normal loop can start
    if(nextColor != 0)
    {
        for (int color = nextColor; color < 8; color++)
        {
          Serial.println("Fade");
          boolean isDone = FadeToColorWriter(startColor, loopColors[color], delayTime);
          if (isDone != true)
          {
            // startAt is already overwritten inside FadeToColorWriter
            isInterrupted = true;
            nextColorInFade = color;
            break;
          }
          else
          {
            memcpy(startColor, loopColors[color], sizeof(startColor));
            delay(delayTime*3); // Waits 3 frame, when it reaches the needed color
          }
        }
        
        nextColor = 0; // To skip this section later on
    }
  
    if(isInterrupted == false)
    {
      for (int color = 0; color < 8; color++)
      {
        Serial.println("Fade");
        boolean isDone = FadeToColorWriter(startColor, loopColors[color], delayTime);
        if (isDone != true)
        {
          // startAt is already overwritten inside FadeToColorWriter
          isInterrupted = true;
          nextColorInFade = color;
          break;
        }
        else
        {
          memcpy(startColor, loopColors[color], sizeof(startColor));
          delay(delayTime*3); // Waits 3 frame, when it reaches the needed color
        }
      }
    }
  } while (isInterrupted == false && WiFi.status() == WL_CONNECTED);

  WriteRGB(off);

  Serial.println();
  Serial.println("Fade interrupted!");
  Serial.println();
}

// Method - Parse the fade properties from UDP Message
void FadeMode(char message[])
{
  int fadeSpeed;
  char * delimittedMsg;

  delayTime = 20;

  // Checking the message to know, its speed customized, or basic
  delimittedMsg = strtok (message, ":");
  if (!strcmp(delimittedMsg, "FadeSpeed")) 
  {
    delimittedMsg = strtok (NULL, ":");
    fadeSpeed = atoi(delimittedMsg);
    
    /* We actually use minor delays between frames,
    but for users the speed is more convenient */
    delayTime = (100 - fadeSpeed);
  }
  
  // Updating status and sending it back as a response
  sprintf(status, "Fade:%d", fadeSpeed);
  AnswerOnUdp(status);
    
  FadeLoop(startAt, defaultFade, nextColorInFade);
}

// Method - Resets state if something was interrupted
void ResetState()
{
  if(strstr(status, "Fade"))
  {
    FadeLoop(startAt, defaultFade, nextColorInFade);
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

  // Open Serial Printing
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();

  // Load the credentials from NVS
  LoadWifiCredentials(ssid, password);

  // Connect to WiFi
  ConnectToWifi(ssid, password);

  // Open UDP
  Udp.begin(port);
}

void loop()
{ 
  // If WiFi gets disconnected, tries to connect again
  if (WiFi.status() != WL_CONNECTED && !WiFi.softAPIP())
  {
    ConnectToWifi(ssid, password);
  }

  // Checking the size of the last incoming UDP packet, as it is empty or not
  if (Udp.parsePacket() || isInterrupted == true)
  {
    isInterrupted = false;

    // Read the UDP packet into a message variable
    int msgLength = Udp.read(udpMessage, 255);
    if(msgLength > 0)
    {
      udpMessage[msgLength] = 0;
    }

    Serial.println();
    Serial.println("UDP Packet Received: ");
    Serial.println(udpMessage);
    Serial.println();
    
    //_____________________________________________

    /*Answers the current status 
    (Status --> Fade, Status --> 255:255:255, Status --> Off)*/
    if(strstr(udpMessage, "Status")) 
    {
      Serial.println("Status Command!");
      AnswerOnUdp(status);
      ResetState();
    }
    
    /*Set Wifi Credentials
    (Credentials:Ssid:Password --> It will connect with(Ssid,Password)*/
    else if(strstr(udpMessage, "Credentials"))
    {
      Serial.println("Credentials Command!");
      SaveWifiCredentials(udpMessage);
    }
    
    //_____________________________________________

    // Fade (Fade --> Normal fade, FadeSpeed:(0-100) --> Customized fade)
    else if(strstr(udpMessage, "Fade")) 
    {
      Serial.println("Fade Command!");
      FadeMode(udpMessage);
    }

    // Color Code (255:255:255 --> White)
    else if(strchr(udpMessage, ':')) 
    {
      Serial.println("Color Code Command!");
      ColorCodeMode(udpMessage);
    }
  }
}
