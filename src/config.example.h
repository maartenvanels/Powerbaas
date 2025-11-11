#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// IMPORTANT: Copy this file to config.h and
// fill in your credentials before building!
// ============================================

// WiFi Configuration
// Option 1: Use WiFiManager (captive portal) - leave wifi_ssid empty
// Option 2: Direct WiFi connection - fill in wifi_ssid and wifi_password
const char* wifi_ssid = "";                  // Your WiFi SSID (leave empty to use WiFiManager)
const char* wifi_password = "";              // Your WiFi password
const char* wifi_hostname = "powerbaas";     // mDNS hostname (accessible as powerbaas.local)

// OTA Configuration
const char* ota_password = "powerbaas123";   // OTA update password (for security)

// Web Authentication
const char* web_username = "admin";          // Web interface username
const char* web_password = "powerbaas123";   // Web interface password

// MQTT Configuration
const char* mqtt_server = "192.168.1.100";   // Your MQTT broker IP address
const int mqtt_port = 1883;                  // MQTT broker port (1883 for standard, 8883 for SSL)
const char* mqtt_user = "";                  // MQTT username (leave empty if no authentication)
const char* mqtt_password = "";              // MQTT password (leave empty if no authentication)

// Device Configuration for Home Assistant
const char* device_name = "Powerbaas";       // Device name shown in Home Assistant
const char* device_id = "powerbaas_p1";      // Unique device ID (change if you have multiple devices)

// MQTT Update Interval
const unsigned long mqttPublishInterval = 10000; // Publish sensor data every X milliseconds (10000 = 10 seconds)

#endif
