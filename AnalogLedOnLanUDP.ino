//Basic
#include <algorithm>

//WIFI
#include "WiFi.h"
#include "WiFiUdp.h"

//LED Default Settings
#define LED_CHANNEL_RED 0
#define LED_CHANNEL_GREEN 1
#define LED_CHANNEL_BLUE 2

#define LEDC_TIMER_13_BIT 13
#define LEDC_BASE_FREQ 5000

#define LED_PIN_RED 17
#define LED_PIN_GREEN 4
#define LED_PIN_BLUE 16

//NVS
#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"


//WiFi Credentials
char* ssid      = "";
char* password  = "";

//UDP
unsigned int port = 2390;
char udpMessage[255];
WiFiUDP Udp;
boolean isInterrupted;
char* delimiterFound;

//Status
char status[25] = "Off";

/*Fade Settings, most of these used for
start fade again if it was interrupted*/
int delayTime;
int startAt[3] = {0, 0, 0};
int nextColorInFade = 0;

int defaultFade[9][3] = //Valid Fade Settings
 {
  {255, 255, 255}, //White
  {255, 0, 0}, // RED
  {0, 255, 0}, // GREEN
  {255, 0, 10}, // PURPLE
  {255, 243, 18}, // YELLOW
  {0, 50, 255}, // LIGHT BLUE
  {255, 165, 0}, // ORANGE
  {192, 255, 62}, // LIGHT GREEN
  {0, 0, 153} // DARK BLUE
 };
 
//_____________________________________________
// Method - Connects to Wifi
void ConnectToWifi() 
{
  int retries = 0;
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  int status = WiFi.begin(ssid, password);

  do
  {
    Serial.print(".");
    delay(4000);
    status = WiFi.begin(ssid, password);
    retries++;
  }while(status != WL_CONNECTED && retries < 10);
  
  if(status != WL_CONNECTED)
  {
    HostSoftAP();
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

//_____________________________________________
//Method - Creates Soft-AP (to modify normal AP credentials on it)
void HostSoftAP()
{
  strcpy(status, "Disconnected");
  WiFi.softAP("LedController");
  IPAddress softIP = WiFi.softAPIP();
  Serial.println("");
  Serial.println("Soft-AP hosted on: ");
  Serial.print(softIP);
  
  int Info[3] = {2,1,1};
  WriteRGB(Info); //Shows a really dark color to tell something changed
}

//_____________________________________________
//Method - Sets Wifi credential variables in Non-volatile storage
void SaveWifiCredentials(char message[])
{
  char* credential;
  
  //NVS Initialize
  esp_err_t err = nvs_flash_init();
  
  const esp_partition_t* nvs_partition = 
  esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
  if(!nvs_partition) printf("FATAL ERROR: No NVS partition found\n");
    
  nvs_handle handler;
  err = nvs_open("storage", NVS_READWRITE, &handler);
  if (err != ESP_OK) printf("FATAL ERROR: Unable to open NVS\n");
  
  //Clean old credentials from NVS
  err = nvs_erase_key(handler, "ssid");
  err = nvs_erase_key(handler, "secret");
  
  err = nvs_commit(handler);
  
  //Parses the message
  credential = strtok (message, ":");

  if (credential != NULL)
  {
    credential = strtok (NULL, ":");
    err = nvs_set_str(handler, "ssid", credential);
    Serial.println("Set SSID: ");
    Serial.println(credential);
  }

  if (credential != NULL)
  {
    credential = strtok (NULL, ":");
    err = nvs_set_str(handler, "secret", credential);
    Serial.println("Set Secret: ");
    Serial.println(credential);
  }
  else //If Wifi secret not provided, set it to empty string
  {
    err = nvs_set_str(handler, "secret", "");
    Serial.println("Set Secret: ");
    Serial.println(credential);
  }
  
  err = nvs_commit(handler);
  Serial.println("");
  Serial.println("Credentials saved to NVS!");
  Serial.println("");
  
  Serial.println("Restarting..");
  delay(2000);
  ESP.restart();
}

//_____________________________________________
//Method - Gets Wifi credential variables from Non-volatile storage
void LoadWifiCredentials()
{
  size_t string_size;
  
  //NVS Initialize
  esp_err_t err = nvs_flash_init();
  
  const esp_partition_t* nvs_partition = 
  esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
  if(!nvs_partition) Serial.println("FATAL ERROR: No NVS partition found\n");
    
  nvs_handle handler;
  err = nvs_open("storage", NVS_READWRITE, &handler);
  if (err != ESP_OK) Serial.println("FATAL ERROR: Unable to open NVS\n");
   
  //Read
  Serial.println("");
  Serial.println("Loading credentials from NVS");
  Serial.println("");
  
  err = nvs_get_str(handler, "ssid", NULL, &string_size);
  char* id = (char*)malloc(string_size);
  err = nvs_get_str(handler, "ssid", id, &string_size);
  
  err = nvs_get_str(handler, "secret", NULL, &string_size);
  char* pw = (char*)malloc(string_size);
  err = nvs_get_str(handler, "secret", pw, &string_size);

  
  Serial.print("SSID: ");
  Serial.println(id);
  Serial.print("Password: ");
  Serial.println(pw);
  
  ssid = id;
  password = pw;
}

//_____________________________________________
//Method - Writes intensity to *LED Color Channel*
void WriteToLedChannel(uint8_t channel, uint32_t value, uint32_t valueMax = 255) 
{
  uint32_t duty = (8191 / valueMax) * _min(value, valueMax);
  ledcWrite(channel, duty);
}

//_____________________________________________
// Method - Writes RGB Array To LED in one step
void WriteRGB(int RGB[])
{
  WriteToLedChannel(LED_CHANNEL_RED, RGB[0]);
  WriteToLedChannel(LED_CHANNEL_GREEN, RGB[1]);
  WriteToLedChannel(LED_CHANNEL_BLUE, RGB[2]);
}

//_____________________________________________
// Method - Parses the color code from UDP Message and writes it to the LED (e.g.: 255:255:255 --> White)
void ColorCodeWriter(char message[])
{
  char * colorIntensity;
  int RGB[3] = {0,0,0};
  
  //Parses the message
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
  
  //Setting Color Code as Status
  sprintf(status, "%d:%d:%d", RGB[0], RGB[1], RGB[2]);
  if(strstr(status, "0:0:0"))
  {
    strcpy(status, "Off");
  }
  WriteRGB(RGB);
}

//_____________________________________________
// Method - Fades between two given color, with a given delay speed between each frame
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

  //Helps to reduce the number of fade steps
  for (int component = 0; component < 3 ; component++)
  {
    howManySteps[component] = colorDifferencesNotNegative[component] / 5;
    if (howManySteps[component] == 0) {
      howManySteps[component] = 1;
    }
    
    increasement[component] = div(colorDifferences[component], howManySteps[component]);
    currentState[component] = currentState[component] + increasement[component].rem;
  }

  // "Step before start", - to reach the perfect color as result,
  // it writes the remenent color differences first to be nearly invisible, still simple
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
    
    //If UDP Received, stop fading
    if (Udp.parsePacket() != 0)
    {
      memcpy(startAt, currentState, sizeof(currentState));
      return false;
    }

    WriteRGB(currentState);
    delay(delTime);
  }
  
  return true;
}

//_____________________________________________
//Method - Looping through fade colors
void FadeLoop(int delayTime, int startColor[3], int loopColors[][3], int nextColor)
{
  int RGB[3] = {0, 0, 0};

  Serial.println();
  Serial.print("In fade loop, delay: ");
  Serial.println(delayTime);
    
  //Fading Process
  do
  {
    //It is used for finishing interrupted fade loops, then the normal loop can start
    if(nextColor != 0)
    {
        for (int color = nextColor; color < 9; color++)
        {
          Serial.println("Fade");
          boolean isDone = FadeToColorWriter(startColor, loopColors[color], delayTime);
          if (isDone != true)
          {
            //startAt is already overwritten inside FadeToColorWriter
            isInterrupted = true;
            nextColorInFade = color;
            break;
          }
          else
          {
            memcpy(startColor, loopColors[color], sizeof(startColor));
            delay(delayTime*3); //Waits 3 frame, when it reaches the needed color
          }
        }
        
        nextColor = 0; // To skip this section later
    }
  
    if(isInterrupted == false)
    {
      for (int color = 0; color < 9; color++)
      {
        Serial.println("Fade");
        boolean isDone = FadeToColorWriter(startColor, loopColors[color], delayTime);
        if (isDone != true)
        {
          //startAt is already overwritten inside FadeToColorWriter
          isInterrupted = true;
          nextColorInFade = color;
          break;
        }
        else
        {
          memcpy(startColor, loopColors[color], sizeof(startColor));
          delay(delayTime*3); //Waits 3 frame, when it reaches the needed color
        }
      }
    }
  } while (isInterrupted == false && WiFi.status() == WL_CONNECTED);

  int off[3] = {0, 0, 0};
  WriteRGB(off);

  Serial.println();
  Serial.println("Fade interrupted!");
  Serial.println();
}

//_____________________________________________
//Method - Parse the fade properties from UDP Message
void FadeParser(char message[])
{
  delayTime = 40;
  char * delimittedMsg;
    
  //Checking the message to know, its speed customized, or basic
  delimittedMsg = strtok (message, ":");
  if (!strcmp(delimittedMsg, "FadeSpeed")) 
  {
    delimittedMsg = strtok (NULL, ":");
      
    /*We actually use minor delays between frames,
    but for users the speed is more convenient*/
    delayTime = 100 - atoi(delimittedMsg);
  }
    
  FadeLoop(delayTime, startAt, defaultFade, nextColorInFade);
}

//_____________________________________________
//Method - Answers the current status to the partner
void AnswerToStatus(IPAddress remoteIP, uint16_t remotePort)
{
  Serial.print("Answering with: ");
  Serial.println(status);
  Serial.print("to: ");
  Serial.println(remoteIP);
  Udp.beginPacket(remoteIP, remotePort);
  Udp.print(status);
  Udp.endPacket(); 
    
  if(strstr(status, "Fade"))
  {
    FadeLoop(delayTime, startAt, defaultFade, nextColorInFade);
  }
}

//_____________________________________________
//Method - Answers the current status to the partner
void AnswerToPing(IPAddress remoteIP, uint16_t remotePort)
{
  Udp.beginPacket(remoteIP, remotePort);
  Udp.print("Pong");
  Udp.endPacket();  
}

//_____________________________________________
void setup()
{
  // Initialize LED Color Channels (R,G,B)
  ledcSetup(LED_CHANNEL_RED, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_RED, LED_CHANNEL_RED);

  ledcSetup(LED_CHANNEL_GREEN, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_GREEN, LED_CHANNEL_GREEN);

  ledcSetup(LED_CHANNEL_BLUE, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN_BLUE, LED_CHANNEL_BLUE);

  // Open Serial Monitor Printing
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();

  LoadWifiCredentials();

  //Connect to WiFi
  ConnectToWifi();

  // Open UDP
  Udp.begin(port);
}

//_____________________________________________
void loop()
{ 
  //If WiFi gets disconnected, tries to connect again
  if (WiFi.status() != WL_CONNECTED && !WiFi.softAPIP())
  {
    ConnectToWifi();
  }

  //Checking the size of the last incoming UDP packet, as it is empty or not
  if (Udp.parsePacket() || isInterrupted == true)
  {
    isInterrupted = false;

    //Read the UDP packet into a message variable
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
      AnswerToStatus(Udp.remoteIP(), 8082);
    }
    
    /*Answers if someone search this controller on broadcast
    (Ping --> Returns "Pong" string to the original sender)*/
    else if(strstr(udpMessage, "Ping"))
    {
      Serial.println("Ping Command!");
      AnswerToPing(Udp.remoteIP(), 8082);
    }
    
    /*Set Wifi Credentials
    (Credentials:Ssid:Password --> It will connect with(Ssid,Password)*/
    else if(strstr(udpMessage, "Credentials"))
    {
      Serial.println("Credentials Command!");
      SaveWifiCredentials(udpMessage);
    }
    
    //_____________________________________________
    
    //Fade (Fade --> Normal fade, FadeSpeed(0-100) --> Customized fade)
    else if(strstr(udpMessage, "Fade")) 
    {
      Serial.println("Fade Command!");
      int fadeSpeedHelper = 100 - delayTime;
      sprintf(status, "Fade:%d", fadeSpeedHelper);
      FadeParser(udpMessage);
    }

    //Color Code (255:255:255 --> White)
    else if(strchr(udpMessage, ':')) 
    {
      Serial.println("Color Code Command!");
      //Status switch lives inside ColorCodeWriter
      ColorCodeWriter(udpMessage);
    }
  }
}
