#include <Arduino.h>
#include <Ticker.h>
#include <PubSubClient.h>
#if defined(ESP32)
#include <WiFi.h>
#include "Wire.h"
#include "DHTesp.h"
#include "BH1750.h"

#endif

const char *ssid = "Clem";
const char *password = "2501961123";
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH "esp32_test/data"
#define MQTT_TOPIC_PUBLISH2 "esp32_test/humidity"
#define MQTT_TOPIC_PUBLISH3 "esp32_test/temperature"
#define MQTT_TOPIC_PUBLISH4 "esp32_test/light"
#define MQTT_TOPIC_SUBSCRIBE "esp32_test/cmd"
DHTesp dht;
BH1750 lightMeter;
#define DHTPIN 4
#define LEDRED 18
#define LEDGREEN 19
#define LEDYELLOW 23
#define PIN_SDA 21
#define PIN_SCL 22

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Ticker timerPublish, temppublish, humiditypublish, lightpublish;
int nMsgCount = 0;

char g_szDeviceId[30];
void WifiConnect();
boolean mqttConnect();
// void onPublishMessage();
void onPublishMessagetemp();
void onPublishMessagehumidity();
void onPublishMessagelight();
void ReadTempHum();

void setup()
{
  Serial.begin(115200);
  delay(100);
  // pinMode(LED_BUILTIN, OUTPUT));
  Serial.printf("Free Memory: %d\n", ESP.getFreeHeap());
  WifiConnect();
  mqttConnect();
  // timerPublish.attach_ms(3000, onPublishMessage);
  temppublish.attach_ms(5000, onPublishMessagetemp);
  humiditypublish.attach_ms(7000, onPublishMessagehumidity);
  lightpublish.attach_ms(4000, onPublishMessagelight);
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(LEDYELLOW, OUTPUT);
  dht.setup(DHTPIN, DHTesp::DHT11);
  Wire.begin(PIN_SDA, PIN_SCL);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
}

void loop()
{
  mqtt.loop();
}


void mqttCallback(char *topic, byte *payload, unsigned int len)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();
  if (strcmp(topic, MQTT_TOPIC_SUBSCRIBE) == 0)
  {
    if (strncmp((char *)payload, "led on", len) == 0)
    {
      digitalWrite(LEDYELLOW, HIGH);
      Serial.println("LED is on");
    }
    else if (strncmp((char *)payload, "led off", len) == 0)
    {
      digitalWrite(LEDYELLOW, LOW);
      Serial.println("LED is off");
    }
  }
}

void onPublishMessagetemp(){
  char SzTopic[50];
  char SzData[50];
  float temperature = dht.getTemperature();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
   // Serial.printf("Temperature: %.2fC, Humidity: %.2f%%", temperature, humidity);
    sprintf(SzTopic, "%s/temperature", MQTT_TOPIC_PUBLISH);
    sprintf(SzData, "%.2f", temperature);
    mqtt.publish(SzTopic, SzData);
    Serial.printf("Temperature : %s\n", SzData);
}
}

void onPublishMessagehumidity(){
  char SzTopic[50];
  char SzData[50];
  float humidity = dht.getHumidity();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
   // Serial.printf("Temperature: %.2fC, Humidity: %.2f%%", temperature, humidity);
    sprintf(SzTopic, "%s/humidity", MQTT_TOPIC_PUBLISH);
    sprintf(SzData, "%.2f", humidity);
    mqtt.publish(SzTopic, SzData);
    Serial.printf("Humidity : %s\n", SzData);
  }
}

void onPublishMessagelight(){
  char SzTopic[50];
  char SzData[50];
  float lux = lightMeter.readLightLevel();
 if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    sprintf(SzTopic, "%s/light", MQTT_TOPIC_PUBLISH);
    sprintf(SzData, "%.2f", lux);
    mqtt.publish(SzTopic, SzData);
    Serial.printf("Lux : %s\n", SzData);
  }

  if (lux>400){
    digitalWrite(LEDRED,HIGH);
    digitalWrite(LEDGREEN,LOW);
    Serial.println("Warning!, Door is open");
  } else{
    digitalWrite(LEDGREEN,HIGH);
    digitalWrite(LEDRED,LOW);
    Serial.println("Door is closed");
  }
}

// void onPublishMessage()
// {
  
//   float humidity = dht.getHumidity();
//   // float temperature = dht.getTemperature();
//   char szMsg[50];
//   sprintf(szMsg, "Hello from %s-%d", g_szDeviceId, nMsgCount++);
//   mqtt.publish(MQTT_TOPIC_PUBLISH, szMsg);
  
//   Serial.printf("Light: %.2f lux\n", lux);

//   sprintf(szMsg, "%f", humidity);
 
// }

boolean mqttConnect()
{
#if defined(ESP32)
  sprintf(g_szDeviceId, "esp32_%08X", (uint32_t)ESP.getEfuseMac());
#endif

  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %sclientId: %s\n", MQTT_BROKER, g_szDeviceId);

  boolean status = mqtt.connect(g_szDeviceId);
  if (status == false)
  {
    Serial.print(" fail, rc=");
    Serial.print(mqtt.state());
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
  Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);

  onPublishMessagelight();
  onPublishMessagehumidity();
  onPublishMessagetemp();
  return mqtt.connected();
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}
