#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

// Method - Connects to Wifi
void ConnectToWifi(char ssid[], char password[]);

// Method - Creates Soft-AP (to modify normal AP credentials on it)
void HostSoftAP();

#endif /* WIFI_HELPER_H */
