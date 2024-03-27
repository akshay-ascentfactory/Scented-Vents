#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi Access Point configuration
#define WIFI_AP_SSID "ESP8266_AP"
#define WIFI_AP_PASSWORD "password"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_SSID_HIDDEN false
#define WIFI_AP_MAX_CONNECTIONS 5
#define WIFI_AP_BEACON_INTERVAL 100
#define WIFI_AP_IP IPAddress(192, 168, 0, 1)
#define WIFI_AP_GATEWAY IPAddress(192, 168, 0, 1)
#define WIFI_AP_NETMASK IPAddress(255, 255, 255, 0)

// MQTT Broker configuration
const char *mqttServer = "act5i3l7e20lx-ats.iot.us-east-1.amazonaws.com";
const int mqttPort = 8883;
const char *mqttClientId = "ESP8266_Client";
const char *mqttTopic = "ota/firmware";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Function prototypes
void connectToWiFi();
void reconnectMQTT();
void callback(char *topic, byte *payload, unsigned int length);

void setup()
{
  Serial.begin(115200);

  connectToWiFi();
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void loop()
{
  if (!mqttClient.connected())
  {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Your main code goes here (if any)
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, WIFI_AP_SSID_HIDDEN, WIFI_AP_MAX_CONNECTIONS);
  WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_NETMASK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void reconnectMQTT()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(mqttClientId))
    {
      Serial.println("connected");

      // Once connected, subscribe to topics
      mqttClient.subscribe(mqttTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Convert payload to a String
  String message;
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Check if the topic is for OTA update
  if (strcmp(topic, mqttTopic) == 0)
  {
    // Extract firmware update URL from the message
    String firmwareUrl = message;

    // Print received firmware URL for debugging
    Serial.print("Received firmware URL: ");
    Serial.println(firmwareUrl);

    // Implement firmware update handling here
    // handleOTAUpdate(firmwareUrl);
  }
}









/*
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP8266HTTPUpdate.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#define WIFI_AP_SSID "ESP8266_AP"
#define WIFI_AP_PASSWORD "password"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_SSID_HIDDEN 0
#define WIFI_AP_MAX_CONNECTIONS 5
#define WIFI_AP_BEACON_INTERVAL 100
#define WIFI_AP_IP "192.168.0.1"
#define WIFI_AP_GATEWAY "192.168.0.1"
#define WIFI_AP_NETMASK "255.255.255.0"

#define MQTT_SERVER "your_aws_mqtt_endpoint"
#define MQTT_PORT 8883
#define MQTT_CLIENT_ID "ESP8266_Client"
#define MQTT_TOPIC "ota/firmware"

#define DHT_PIN D4
#define DHT_TYPE DHT11

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);

ESP8266WebServer server(80);

void handleOTAUpdate(String firmwareUrl)
{
  // Download firmware from the provided URL
  HTTPClient http;
  http.begin(firmwareUrl);

  Serial.println("Downloading firmware...");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    // Start firmware update process
    Serial.println("Start firmware update process...");
    if (Update.begin(http.getSizeTotal()))
    {
      // Stream the firmware to the ESP8266
      WiFiClient stream = http.getStream();
      while (stream.available())
      {
        uint8_t data = stream.read();
        if (Update.write(data) != 1)
        {
          Serial.println("Firmware update write failed!");
          Update.end();
          http.end();
          return;
        }
      }

      // Finish firmware update process
      if (Update.end(true))
      {
        Serial.println("Firmware update successful!");
        ESP.restart();
      }
      else
      {
        Serial.println("Firmware update failed!");
      }
    }
    else
    {
      Serial.println("Firmware update begin failed!");
    }
  }
  else
  {
    Serial.println("Failed to download firmware!");
  }

  http.end();
}

void handleFileUpload()
{
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    Serial.printf("Start uploading firmware: %s\n", upload.filename.c_str());
    if (!Update.begin())
    {
      Serial.println("Firmware update begin failed!");
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
    {
      Serial.println("Firmware update write failed!");
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (Update.end(true))
    {
      Serial.println("Firmware update successful!");
      ESP.restart();
    }
    else
    {
      Serial.println("Firmware update failed!");
    }
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, WIFI_AP_SSID_HIDDEN, WIFI_AP_MAX_CONNECTIONS);
  WiFi.softAPConfig(IPAddress(WIFI_AP_IP), IPAddress(WIFI_AP_GATEWAY), IPAddress(WIFI_AP_NETMASK));
  Serial.println("Access Point started");

  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println("File system mounted");

  File file = SPIFFS.open("/index.html", "r");
  if (!file)
  {
    Serial.println("Failed to open index.html file");
    return;
  }
  String htmlContent = file.readString();
  file.close();
  Serial.println("File loaded");

  server.on("/", [&htmlContent]()
            { server.send(200, "text/html", htmlContent); });

  server.on("/disconnect", []()
            {
    server.send(200, "text/plain", "Disconnecting...");
    delay(1000);
    WiFi.softAPdisconnect(true); });

  server.on(
      "/upload", HTTP_POST, []()
      { server.send(200, "text/plain", "OK"); },
      handleFileUpload);

  server.on("/ota", HTTP_POST, handleOTAUpdate);

  server.on("/data", []()
            {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (!isnan(temperature) && !isnan(humidity)) {
      String data = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
      server.send(200, "application/json", data);
    } else {
      server.send(500, "text/plain", "Error reading sensor data");
    } });

  server.begin();
  Serial.println("HTTP server started");

  dht.begin();

  connectToWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);

  Serial.println("OTA update available at: " + WiFi.localIP().toString());
}

void loop()
{
  server.handleClient();
  if (!mqttClient.connected())
  {
    reconnectMQTT();
  }
  mqttClient.loop();
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void reconnectMQTT()
{
  Serial.print("Attempting MQTT connection...");
  if (mqttClient.connect(MQTT_CLIENT_ID))
  {
    Serial.println("connected");
    mqttClient.subscribe(MQTT_TOPIC);
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
    delay(5000);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Convert payload to a String
  String message;
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Check if the topic is for OTA update
  if (strcmp(topic, MQTT_TOPIC) == 0)
  {
    // Extract firmware update URL from the message
    String firmwareUrl = message;

    // Print received firmware URL for debugging
    Serial.print("Received firmware URL: ");
    Serial.println(firmwareUrl);
*/
    // Trigger OTA update using the received firmware URL
    handleOTAUpdate(firmwareUrl);
  }
}
