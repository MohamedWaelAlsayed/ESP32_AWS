#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>

// Pin definitions
#define SENSOR_PIN 35 // Pin number for the soil moisture sensor

// AWS IoT configurations
#define AWS_IOT_PUBLISH_TOPIC   "farm1/esp32/pub"
// #define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// Define Temperature Sensor
// Ds18B20 Temperature sensor
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void connectAWS()
{
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Wi-Fi");

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  // client.setCallback(messageHandler);

  Serial.print("Connecting to AWS IoT");
  while (!client.connected()) {
    if (client.connect(THINGNAME)) {
      Serial.println("\nConnected to AWS IoT");
      // client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    }
    else {
      Serial.print(".");
      delay(1000);
    }
  }
}

void publishMessage(float soilMoisture, float temperature)
{
  // Create a JSON document
  StaticJsonDocument<200> doc;
  doc["soil_moisture"] = soilMoisture;
  doc["temperature"] = temperature;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // serialize the JSON document to a string

  // Publish the message
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Published message: " + String(jsonBuffer));
}

// void messageHandler(char* topic, byte* payload, unsigned int length)
// {
//   Serial.print("Received message on topic: ");
//   Serial.println(topic);
//   Serial.print("Message: ");
//   for (int i = 0; i < length; i++) {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println();
// }

void setup()
{
  Serial.begin(115200);
  sensors.begin();

  // Connect to AWS IoT
  connectAWS();
}

void loop()
{
  // Read soil moisture
  float soilMoisture = analogRead(SENSOR_PIN);
  soilMoisture = map(soilMoisture, 0, 4095, 0, 100); // convert the raw reading to a percentage

  // Read temperature
  // sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  Serial.print("Soil moisture: ");
  Serial.print(soilMoisture);
  Serial.print("% Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  // Publish the sensor readings to AWS IoT
  publishMessage(soilMoisture, temperature);
  client.loop();

  // Wait for some time before taking the next reading
  delay(20000);
}