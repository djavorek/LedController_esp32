#ifndef NVS_HELPER_H
#define NVS_HELPER_H

// Sets Wifi credential variables in Non-volatile storage
void SaveWifiCredentials(char message[]);

// Gets Wifi credential variables from Non-volatile storage
void LoadWifiCredentials(char ssid[], int lengthSsid, char password[], int lengthPassword);

#endif /* NVS_HELPER_H */
