#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

// Creates hotspot (to set credentials on)
void HostSoftAP(char* deviceName);

// Connects to WiFi with the given parameters
void ConnectToWifi(char ssid[], char password[]);

// If hotspot gets hosted it will remain true until the next restart
boolean isHotspotHosted();

#endif /* WIFI_HELPER_H */

