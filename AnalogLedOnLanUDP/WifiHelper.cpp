#include "HardwareSerial.h"
#include "WiFi.h"

#include "ColorHelper.h"

boolean hotspotHosted = false;

void HostSoftAP(char* deviceName)
{
  char softApSSID[50] = "Led_";
  strncat(softApSSID, deviceName, sizeof(softApSSID) - strlen(softApSSID)); 
  WiFi.softAP(softApSSID, deviceName);
  IPAddress softIP = WiFi.softAPIP();
  Serial.println("");
  Serial.println("Soft-AP hosted on: ");
  Serial.print(softIP);
}

void ConnectToWifi(char ssid[], char password[]) 
{
  int retries = 0;
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.disconnect(0);
  int status = WiFi.begin(ssid, password);

  do
  {
    Serial.print(".");
    delay(4000);
    status = WiFi.begin(ssid, password);
    retries++;
  }while(status != WL_CONNECTED && retries < 10);
  
  if(status == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("");
    Serial.println("Cannot connect to WiFI");
    
    // Turns LED off
    int off[] = {0, 0, 0};
    WriteRGB(off);
  }
}

boolean isHotspotHosted()
{
  return hotspotHosted;
}



