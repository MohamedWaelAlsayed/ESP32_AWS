#include "secrets.h"
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include "time.h"


// Pin definitions
#define SENSOR_PIN 39 // Pin number for the soil moisture sensor
// AWS IoT configurations
#define AWS_IOT_PUBLISH_TOPIC   "farms/farm1/"
// #define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// set number of messages
  int no_of_messages = 0;

// Define Temperature Sensor
// Ds18B20 Temperature sensor
// GPIO where the DS18B20 is connected to
const int oneWireBus = 13;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Network time Protocl Server
const char* ntpServer = "pool.ntp.org";

void connectAWS()
{
  // Connect to Wi-Fi
  // WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("\nConnecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    // WiFi.reconnect();
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

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void publishMessage(float soilMoisture, float temperature, int timestamp, int* no_of_messages)
{
  // Create a JSON document
  StaticJsonDocument<512> doc;
  doc["moisture"] = soilMoisture;
  doc["temperature"] = temperature;
  doc["device_id"] = "arn:aws:iot:us-east-1:404548260653:thing/ESP32_Farm1";
  doc["timestamp"] = timestamp;
  // doc["timestamp"] =  "16546";
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // serialize the JSON document to a string

  // Publish the message
  
  boolean returned = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  if (returned == 1){
    Serial.print("Sent Done\n");
    *no_of_messages = *no_of_messages + 1;
    
     Serial.println(String(client.state()));
     Serial.println("number of sent messages : " + String(*no_of_messages) );
     Serial.println("Published message: " + String(jsonBuffer));
     Serial.println("-----------------------------------------------------------------------"); 
  }else{
    Serial.print("\nFailed\n");
    Serial.println(String(client.state()));
    Serial.println("Message not sent");
  }
  // Serial.print("returned "+ returned);
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

//Configure times   
  configTime(0, 0, ntpServer);

  // Connect to AWS IoT
  connectAWS();

}

void loop()
{
  // // Check if the connection to AWS IoT is still active and reconnect if necessary
  // if (!client.connected()) {
  //   connectAWS();
  // }

  // Read soil moisture
  float soilMoisture = analogRead(SENSOR_PIN);
  soilMoisture = map(soilMoisture, 0, 4095, 0, 100); // convert the raw reading to a percentage

  // Read temperature
  // sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // get time
  int timestamp = getTime();
  Serial.println("\nCaptured Reading");
  Serial.print("Soil moisture: ");
  Serial.print(soilMoisture);
  Serial.print("%   Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

    // Check if the connection to AWS IoT is still active and reconnect if necessary
  if (!client.connected()) {
    WiFi.disconnect();
    connectAWS();
  }
  // Publish the sensor readings to AWS IoT
  publishMessage(soilMoisture, temperature, timestamp, &no_of_messages);
  client.loop();

  // Wait for some time before taking the next reading
  delay(5000);
}
