// Basics
#include "Arduino.h"

// WIFI
#include "WiFi.h"
#include "WiFiUdp.h"

// HEADERS
#include "LedColor.h"
#include "WiFiHelper.h"

void ConnectToWifi(char ssid[], char password[]) 
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

void HostSoftAP()
{
  WiFi.softAP("LedController");
  IPAddress softIP = WiFi.softAPIP();
  Serial.println("");
  Serial.println("Soft-AP hosted on: ");
  Serial.print(softIP);
  
  int Info[3] = {2,1,1};
  WriteRGB(Info); //Shows a really dark color to tell something changed
}
