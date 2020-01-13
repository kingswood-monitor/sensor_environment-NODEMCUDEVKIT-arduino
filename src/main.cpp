#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Ticker.h"

#include "CompositeSensor.h"
#include "sensor-utils.h"

#include "secret.h"

#define REFRESH_MILLIS 3000 // sensor refresh period (milliseconds)

#define CLIENT_ID 4 // See Location Scheme for location mapping
#define FIRMWARE_NAME "Environment Sensor"
#define FIRMWARE_SLUG "sensor_environment-NODEMCUDEVKIT-arduino"
#define FIRMWARE_VERSION "0.2"
#define DEVICE_ID "ESP8266-04"
// WiFi credentials

// Hardware configuration
#define LED_RED 16

void setup_wifi();
void reconnect();
void publish_float(int client, char *topic, float val);
void publish_int(int client, char *topic, int val);
void blink_cb();
void updateReadings_cb();

// WiFi / MQTT configuration
IPAddress mqtt_server(192, 168, 1, 30);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Timers
Ticker blinkTimer(blink_cb, 100, 2);
Ticker updateReadings(updateReadings_cb, REFRESH_MILLIS);

// Initialise sensors
CompositeSensor mySensor;

void setup()
{
  Serial.begin(57600);
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW); // inverted

  utils::printBanner(FIRMWARE_NAME, FIRMWARE_SLUG, FIRMWARE_VERSION, DEVICE_ID);

  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);

  mySensor.begin();
  updateReadings.start();
}

void loop()
{
  if (!mqttClient.connected())
    reconnect();

  mqttClient.loop();
  blinkTimer.update();
  updateReadings.update();
}

/** Callback funciton for main loop to update and publish readings 
*/
void updateReadings_cb()
{
  CompositeSensor::SensorReadings readings = mySensor.readSensors();

  publish_float(CLIENT_ID, "data/temperature", readings.temp);
  publish_int(CLIENT_ID, "data/humidity", readings.humidity);
  publish_int(CLIENT_ID, "data/co2", readings.co2);

  char buf[100];
  sprintf(buf, "T:%.1f, H:%.0f%, CO2:%d", readings.temp, readings.humidity, readings.co2);
  Serial.println(buf);

  digitalWrite(LED_RED, LOW); // inverted
  blinkTimer.start();
}

void blink_cb()
{
  digitalWrite(LED_RED, HIGH); // inverted
}

/************** WiFi / MQTT  *******************************************************************/

void setup_wifi()
{

  delay(10);
  digitalWrite(LED_BUILTIN, HIGH); // inverted
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID_NAME);

  WiFi.begin(SSID_NAME, SSID_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);

    Serial.print(".");
  }
  digitalWrite(LED_BUILTIN, LOW); // inverted

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(DEVICE_ID))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
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

void publish_float(int client, char *topic, float val)
{
  char val_buf[10];
  char topic_buf[20];

  sprintf(topic_buf, "%d/%s/", client, topic);
  sprintf(val_buf, "%.1f", val);

  mqttClient.publish(topic_buf, val_buf);
}

void publish_int(int client, char *topic, int val)
{
  char val_buf[10];
  char topic_buf[20];

  sprintf(topic_buf, "%d/%s/", client, topic);
  sprintf(val_buf, "%d", val);

  mqttClient.publish(topic_buf, val_buf);
}

/***********************************************************************************************/