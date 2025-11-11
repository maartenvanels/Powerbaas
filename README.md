# Powerbaas

## What is it?
Powerbaas Arduino library for smart meter P1 shield<br />
You can order your P1 Powerbaas shield on https://www.powerbaas.nl

## What can you do with it?
With this shield and library you are able to:
- Connect an ESP32 D1 R32 to your smart meter P1 connector
- Read current power usage
- Read power exported to grid during peak hours
- Read power exported to grid during off-peak hours
- Read power imported from grid during peak hours
- Read power imported from grid during off-peak hours
- Read gas used

## Installation

### PlatformIO (Recommended)
1. Clone this repository or download it
2. Open the project folder in PlatformIO
3. Create your configuration file:
   ```bash
   # Copy the example config file
   cp src/config.example.h src/config.h
   ```
4. Edit `src/config.h` and configure your settings:
   ```cpp
   // WiFi Configuration (optional - leave empty to use WiFiManager)
   const char* wifi_ssid = "YourWiFiName";      // Your WiFi SSID
   const char* wifi_password = "YourPassword";  // Your WiFi password
   const char* wifi_hostname = "powerbaas";     // mDNS hostname

   // MQTT Configuration
   const char* mqtt_server = "192.168.1.100";   // Your MQTT broker IP
   const int mqtt_port = 1883;                  // MQTT port
   const char* mqtt_user = "";                  // MQTT username (optional)
   const char* mqtt_password = "";              // MQTT password (optional)

   // Device Configuration
   const char* device_name = "Powerbaas";       // Device name in Home Assistant
   const char* device_id = "powerbaas_p1";      // Unique device ID
   ```

   **WiFi Options:**
   - **Option 1 (Direct):** Fill in `wifi_ssid` and `wifi_password` for direct connection
   - **Option 2 (Captive Portal):** Leave `wifi_ssid` empty to use WiFiManager. The device will create an access point named "Powerbaas" where you can configure WiFi through a web interface
5. Build and upload:
   ```bash
   pio run --target upload
   ```

**Note:** The `src/config.h` file is ignored by git to keep your credentials secure. Always use `src/config.example.h` as a template.

### Arduino IDE
1. Download this library and add it to your Arduino libraries folder
2. Open one of the examples from File → Examples → Powerbaas
3. Install required dependencies through Library Manager:
   - WiFiManager by tzapu
   - PubSubClient by Nick O'Leary
   - ArduinoJson by Benoit Blanchon

## Features

### MQTT and Home Assistant Integration
The main program includes full MQTT support with Home Assistant auto-discovery:
- Automatic device registration in Home Assistant
- 12 sensors automatically created:
  - Power Usage (W)
  - Power Delivered High/Low (kWh)
  - Power Return High/Low (kWh)
  - Gas consumption (m³)
  - Voltage per phase (V)
  - Current per phase (A)
- Real-time data publishing every 10 seconds
- Automatic reconnection handling

## Added in version 1.3.13
- Fixed a bug in HomeAutomation example json formatting

## Added in version 1.3.12
- Fixed a bug in HomeAutomation example json formatting

## Added in version 1.3.11
- Fixed a bug in powerUsage when importing and exporting at same time on different phases

## Added in version 1.3.10
- Fixed buffer size setting

## Added in version 1.3.9
- Fixed buffer size setting

## Added in version 1.3.8
- Fixed double parsing some more

## Added in version 1.3.7
- Fixed double parsing

## Added in version 1.3.6
- Fixed float overflow

## Added in version 1.3.5
- Improved parsing gas meter reading

## Added in version 1.3.4
- Improved home automation example

## Added in version 1.3.3
- Improved home automation example
- Only show current sensor data in JSON when sensor is connected on startup

## Added in version 1.3.2
- Home automation example

## Added in version 1.3.1
- Timestamp

## Added in version 1.3.0
- Current per phase
- Actual

## Added in version 1.2.0
- Support for Belgian Meters

## Added in version 1.1.0
- Support for KlikAanKlikUit 433Mhz socket switches
- Turn switch on or off via webserver button
- Let Powerbaas turn switch on/off based on power usage
- Let Powerbaas turn switch on/off based on time of day
- Fully pairable and configurable via webserver

## Added in version 1.0.2
- MQTT example

## Added in version 1.0.1
- Webserver example
