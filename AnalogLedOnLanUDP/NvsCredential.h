#ifndef NVS_CREDENTIAL_H
#define NVS_CREDENTIAL_H

// Method - Sets Wifi credential variables in Non-volatile storage
void SaveWifiCredentials(char message[]);

// Method - Gets Wifi credential variables from Non-volatile storage
void LoadWifiCredentials(char ssid[], char password[]);

#endif /* NVS_CREDENTIAL_H */
