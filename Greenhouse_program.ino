#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Adafruit_NeoPixel.h>

#define WIFI_SSID "Amitabh Mama"
#define WIFI_PASS "gearboys@123"
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define DATA_TOPIC "greenhouse/data"
#define LED_CONTROL_TOPIC "greenhouse/led"
#define DHT_PIN 5
#define DHT_TYPE DHT11
#define NEOPIXEL_PIN 2
#define NUM_PIXELS 1

DHT sensor(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastReading = 0;
const unsigned long READING_INTERVAL = 10000; // 10 seconds
void setup() {
  Serial.begin(115200);
  Serial.println("Starting greenhouse monitor...");
  sensor.begin();
  strip.begin();
  strip.setBrightness(150); // not too bright
  strip.show();
  setupWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(handleMQTTMessage);
  Serial.println("Setup complete!");
}
void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID); 
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());
}
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
       String clientId = "ESP32Client-" + String(random(0xffff), HEX); 
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe(LED_CONTROL_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}
void handleMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Received: " + message);
  
  if (String(topic) == LED_CONTROL_TOPIC) {
    changeLEDColor(message);
  }
}
void changeLEDColor(String color) {
  color.toLowerCase();
  if (color == "green") {
    strip.setPixelColor(0, strip.Color(0, 255, 0));
  } else if (color == "red") {
    strip.setPixelColor(0, strip.Color(255, 0, 0));
  } else if (color == "blue") {
    strip.setPixelColor(0, strip.Color(0, 0, 255));
  } else if (color == "off") {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
  } else {
    strip.setPixelColor(0, strip.Color(255, 255, 0));
  } 
  strip.show();
}
void loop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop(); 
  if (millis() - lastReading >= READING_INTERVAL) {
    readAndPublishSensorData();
    lastReading = millis();
  }
  delay(100); 
}

void readAndPublishSensorData() {
  float temp = sensor.readTemperature();
  float humid = sensor.readHumidity();
  if (isnan(temp) || isnan(humid)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  String payload = "{\"temp\":" + String(temp, 1) + 
                  ",\"humidity\":" + String(humid, 1) + "}";
  
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("°C, Humidity: ");
  Serial.print(humid);
  Serial.println("%");
  if (mqttClient.publish(DATA_TOPIC, payload.c_str())) {
    Serial.println("Data published successfully");
  } else {
    Serial.println("Failed to publish data");
  }
}