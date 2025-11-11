#include <Powerbaas.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <LittleFS.h>
#include "config.h"

// MQTT Topics
String mqtt_base_topic = "homeassistant/sensor/" + String(device_id);
String mqtt_state_topic = String(device_id) + "/state";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

Powerbaas powerbaas(true);
MeterReading meterReading;
WebServer server(80);

unsigned long lastMqttPublish = 0;
bool mqttDiscoverySent = false;

// Forward declarations
void setupWifi();
void setupOTA();
void setupMqtt();
void setupPowerbaas();
void setupWebserver();
void setupWebOTA();
void reconnectMqtt();
void publishHomeAssistantDiscovery();
void publishSensorData();
String statusJson();
bool checkWebAuth();

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

  Serial.println("\n\nPowerbaas starting...");

  // Initialize LittleFS with auto-format if needed
  Serial.println("Mounting LittleFS...");
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS mount failed, trying to format...");
    if(!LittleFS.format()) {
      Serial.println("LittleFS format failed!");
      Serial.println("Continuing without LittleFS...");
    } else {
      Serial.println("LittleFS formatted successfully");
      if(!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed after format, continuing anyway...");
      } else {
        Serial.println("LittleFS mounted successfully after format");
      }
    }
  } else {
    Serial.println("LittleFS mounted successfully");
  }

  setupWifi();
  setupOTA();
  setupMqtt();
  setupPowerbaas();
  setupWebserver();
  setupWebOTA();

  Serial.println("Setup complete!");
}

void setupWifi() {
  WiFi.mode(WIFI_STA);

  // Check if WiFi credentials are provided in config
  if (strlen(wifi_ssid) > 0) {
    // Direct WiFi connection
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nFailed to connect to WiFi. Falling back to WiFiManager...");
      WiFiManager wm;
      bool res = wm.autoConnect("Powerbaas");
      if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
      }
    } else {
      Serial.println("\nConnected to WiFi!");
    }
  } else {
    // Use WiFiManager (captive portal)
    Serial.println("No WiFi credentials in config. Starting WiFiManager...");
    WiFiManager wm;
    bool res = wm.autoConnect("Powerbaas");

    if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
    }
    else {
      Serial.println("Connected...yeey :)");
    }
  }

  // Start mDNS
  Serial.println("");
  if (MDNS.begin(wifi_hostname)) {
    Serial.print("Connect to webserver: http://");
    Serial.print(wifi_hostname);
    Serial.println(".local");
  }
  Serial.print("Connect to webserver: http://");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  ArduinoOTA.setHostname(wifi_hostname);
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("OTA Hostname: ");
  Serial.println(wifi_hostname);
}

void setupPowerbaas() {
  powerbaas.onMeterReading([](const MeterReading& _meterReading) {
    meterReading = _meterReading;
  });
  powerbaas.setup();
}

bool checkWebAuth() {
  if (!server.authenticate(web_username, web_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void setupWebserver() {

  // Handle index - protected
  server.on("/", []() {
    if (!checkWebAuth()) return;
    server.send(200, "application/json", statusJson());
  });

  // Reboot - protected
  server.on("/reboot", []() {
    if (!checkWebAuth()) return;
    server.send(200, "text/html", "Rebooting...");
    delay(1000);
    ESP.restart();
  });

  // Public health check endpoint (no auth required)
  server.on("/health", []() {
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void setupWebOTA() {
  // OTA Update page - protected
  server.on("/update", HTTP_GET, []() {
    if (!checkWebAuth()) return;

    String html = "<html><head><title>Powerbaas OTA Update</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;max-width:600px;}";
    html += "h1{color:#333;}";
    html += "form{margin:20px 0;}";
    html += "input[type=file]{padding:10px;width:100%;}";
    html += "input[type=submit]{background:#4CAF50;color:white;padding:10px 20px;border:none;cursor:pointer;width:100%;margin-top:10px;}";
    html += "input[type=submit]:hover{background:#45a049;}";
    html += ".info{background:#e7f3fe;border-left:4px solid #2196F3;padding:10px;margin:10px 0;}";
    html += "</style></head><body>";
    html += "<h1>Powerbaas OTA Update</h1>";
    html += "<div class='info'><strong>Info:</strong> Upload new firmware (.bin file). Device will reboot after update.</div>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update' accept='.bin' required><br>";
    html += "<input type='submit' value='Update Firmware'>";
    html += "</form>";
    html += "<br><a href='/'>Back to Home</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  // Handle firmware upload - protected
  server.on("/update", HTTP_POST, []() {
    if (!checkWebAuth()) return;

    server.sendHeader("Connection", "close");
    server.send(200, "text/html", (Update.hasError()) ?
      "<html><body><h1>Update Failed</h1><p>Please try again.</p><a href='/update'>Back</a></body></html>" :
      "<html><body><h1>Update Success!</h1><p>Device rebooting...</p></body></html>");
    delay(1000);
    ESP.restart();
  }, []() {
    if (!server.authenticate(web_username, web_password)) {
      return;
    }
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
}

void loop() {
  ArduinoOTA.handle();
  powerbaas.receive();
  server.handleClient();

  // MQTT reconnect indien nodig
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  mqttClient.loop();

  // Publiceer sensor data naar MQTT
  unsigned long currentMillis = millis();
  if (currentMillis - lastMqttPublish >= mqttPublishInterval) {
    publishSensorData();
    lastMqttPublish = currentMillis;
  }
}

void setupMqtt() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(512); // Verhoog buffer voor discovery messages
  Serial.print("MQTT broker configured: ");
  Serial.println(mqtt_server);
}

void reconnectMqtt() {
  static unsigned long lastAttempt = 0;
  unsigned long now = millis();

  // Probeer niet te vaak opnieuw te verbinden
  if (now - lastAttempt < 5000) {
    return;
  }
  lastAttempt = now;

  Serial.print("Connecting to MQTT broker...");

  String clientId = "ESP32-" + String(device_id);
  bool connected = false;

  if (strlen(mqtt_user) > 0) {
    connected = mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password);
  } else {
    connected = mqttClient.connect(clientId.c_str());
  }

  if (connected) {
    Serial.println("connected!");
    mqttDiscoverySent = false; // Reset discovery flag
    publishHomeAssistantDiscovery();
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void publishHomeAssistantDiscovery() {
  if (mqttDiscoverySent) return;

  Serial.println("Publishing Home Assistant discovery messages...");

  // Array met alle sensoren
  struct SensorConfig {
    const char* name;
    const char* key;
    const char* unit;
    const char* deviceClass;
    const char* icon;
  };

  SensorConfig sensors[] = {
    {"Power Usage", "powerUsage", "W", "power", "mdi:flash"},
    {"Power Delivered High", "powerDeliverHigh", "kWh", "energy", "mdi:transmission-tower-export"},
    {"Power Delivered Low", "powerDeliverLow", "kWh", "energy", "mdi:transmission-tower-export"},
    {"Power Return High", "powerReturnHigh", "kWh", "energy", "mdi:transmission-tower-import"},
    {"Power Return Low", "powerReturnLow", "kWh", "energy", "mdi:transmission-tower-import"},
    {"Gas", "gas", "m³", "gas", "mdi:fire"},
    {"Voltage L1", "voltageL1", "V", "voltage", "mdi:sine-wave"},
    {"Voltage L2", "voltageL2", "V", "voltage", "mdi:sine-wave"},
    {"Voltage L3", "voltageL3", "V", "voltage", "mdi:sine-wave"},
    {"Current L1", "currentL1", "A", "current", "mdi:current-ac"},
    {"Current L2", "currentL2", "A", "current", "mdi:current-ac"},
    {"Current L3", "currentL3", "A", "current", "mdi:current-ac"}
  };

  for (int i = 0; i < 12; i++) {
    StaticJsonDocument<512> doc;

    doc["name"] = String(device_name) + " " + sensors[i].name;
    doc["unique_id"] = String(device_id) + "_" + sensors[i].key;
    doc["state_topic"] = mqtt_state_topic;
    doc["value_template"] = "{{ value_json." + String(sensors[i].key) + " }}";
    doc["unit_of_measurement"] = sensors[i].unit;
    doc["device_class"] = sensors[i].deviceClass;
    doc["icon"] = sensors[i].icon;

    // Device info
    JsonObject device = doc.createNestedObject("device");
    device["identifiers"][0] = device_id;
    device["name"] = device_name;
    device["model"] = "P1 Smart Meter Shield";
    device["manufacturer"] = "Powerbaas";

    String topic = "homeassistant/sensor/" + String(device_id) + "/" + sensors[i].key + "/config";
    String payload;
    serializeJson(doc, payload);

    if (mqttClient.publish(topic.c_str(), payload.c_str(), true)) {
      Serial.print("  Published: ");
      Serial.println(sensors[i].name);
    } else {
      Serial.print("  Failed: ");
      Serial.println(sensors[i].name);
    }

    delay(100); // Kleine delay tussen publicaties
  }

  mqttDiscoverySent = true;
  Serial.println("Discovery messages sent!");
}

void publishSensorData() {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<512> doc;

  doc["timestamp"] = meterReading.timestamp;
  doc["powerUsage"] = meterReading.powerUsage;
  doc["powerDeliverHigh"] = meterReading.powerDeliverHigh;
  doc["powerDeliverLow"] = meterReading.powerDeliverLow;
  doc["powerReturnHigh"] = meterReading.powerReturnHigh;
  doc["powerReturnLow"] = meterReading.powerReturnLow;
  doc["gas"] = meterReading.gas;
  doc["voltageL1"] = meterReading.voltageL1;
  doc["voltageL2"] = meterReading.voltageL2;
  doc["voltageL3"] = meterReading.voltageL3;
  doc["currentL1"] = meterReading.currentL1;
  doc["currentL2"] = meterReading.currentL2;
  doc["currentL3"] = meterReading.currentL3;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish(mqtt_state_topic.c_str(), payload.c_str())) {
    Serial.println("Sensor data published to MQTT");
  }
}

String statusJson() {
  String json = "{\r\n  \"meterReading\": {";
  json += "\r\n    \"timestamp\": \"" + String(meterReading.timestamp) + "\"";
  json += ",\r\n    \"powerUsage\": " + String(meterReading.powerUsage);
  json += ",\r\n    \"powerDeliverHigh\": " + String(meterReading.powerDeliverHigh);
  json += ",\r\n    \"powerDeliverLow\": " + String(meterReading.powerDeliverLow);
  json += ",\r\n    \"powerReturnHigh\": " + String(meterReading.powerReturnHigh);
  json += ",\r\n    \"powerReturnLow\": " + String(meterReading.powerReturnLow);
  json += ",\r\n    \"gas\": " + String(meterReading.gas);
  json += ",\r\n    \"voltageL1\": " + String(meterReading.voltageL1);
  json += ",\r\n    \"voltageL2\": " + String(meterReading.voltageL2);
  json += ",\r\n    \"voltageL3\": " + String(meterReading.voltageL3);
  json += ",\r\n    \"currentL1\": " + String(meterReading.currentL1);
  json += ",\r\n    \"currentL2\": " + String(meterReading.currentL2);
  json += ",\r\n    \"currentL3\": " + String(meterReading.currentL3);
  json += "\r\n  }";
  json += "\r\n}";

  return json;
}