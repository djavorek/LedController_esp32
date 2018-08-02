#include "Arduino.h"
#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

// Method - Sets Wifi credential variables in Non-volatile storage
void SaveWifiCredentials(char message[])
{
  char* credential;
  
  // NVS Initialize
  esp_err_t err = nvs_flash_init();
  
  const esp_partition_t* nvs_partition = 
  esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
  if(!nvs_partition) Serial.println("FATAL ERROR: No NVS partition found\n");
  else
  {
    nvs_handle handler;
    err = nvs_open("storage", NVS_READWRITE, &handler);
    if (err != ESP_OK) Serial.println("FATAL ERROR: Unable to open NVS\n");
    else
    {
      // Parses the message
      credential = strtok (message, ":");
    
      if (credential != NULL)
      {
        // Clean old credentials from NVS
        err = nvs_erase_key(handler, "ssid");
        err = nvs_erase_key(handler, "secret");
        err = nvs_commit(handler);
        
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
      else // If Wifi secret not provided, set it to empty string
      {
        err = nvs_set_str(handler, "secret", "");
        Serial.println("Set Secret: ");
        Serial.println(credential);
      }
      
      err = nvs_commit(handler);
      Serial.println("");
      Serial.println("Credentials saved to NVS!");
      Serial.println("");
    }
  }

  Serial.println("Restarting..");
  delay(2000);
  ESP.restart();
}

void LoadWifiCredentials(char ssid[], int lengthSsid, char password[], int lengthPassword)
{
  size_t string_size;
  
  // NVS Initialize
  esp_err_t err = nvs_flash_init();
  
  const esp_partition_t* nvs_partition = 
  esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
  if(!nvs_partition) Serial.println("FATAL ERROR: No NVS partition found\n");
    
  nvs_handle handler;
  err = nvs_open("storage", NVS_READWRITE, &handler);
  if (err != ESP_OK) Serial.println("FATAL ERROR: Unable to open NVS\n");
   
  // Reading from NVS
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
  
  ssid[0] = '\0';
  strncat(ssid, id, lengthSsid - 1);
  password[0] = '\0';
  strncat(password, pw, lengthPassword - 1);
}
