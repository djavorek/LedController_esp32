// Basics
#include "HardwareSerial.h"
#include <string.h>

// WIFI
#include "WiFi.h"
#include "WiFiUdp.h"

// HEADERS
#include "ColorHelper.h"

void HostSoftAP(char* deviceName)
{
  char softApSSID[50] = "Led_";
  strncat(softApSSID, deviceName, sizeof(softApSSID) - 14); //Magic number: Length of 'LedController '
  WiFi.softAP(softApSSID);
  IPAddress softIP = WiFi.softAPIP();
  Serial.println("");
  Serial.println("Soft-AP hosted on: ");
  Serial.print(softIP);
  
  int Info[3] = {40,40,40};
  WriteRGB(Info); //Shows a dark color to tell something changed
}

void ConnectToWifi(char ssid[], char password[], char* deviceName) 
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
    HostSoftAP(deviceName);
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}


