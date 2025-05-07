#include <WiFi.h>
#include "WiFiClient.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
 
#define WLAN_SSID "WIFI6-2.4G"  //wifi名称
#define WLAN_PASS "200146dd"  //wifi密码
 
 
#define MQTT_SERVER     "broker.emqx.io" //服务器地址，这里使用的是公网所以下面的用户和密码可以随便
 
#define MQTT_SERVERPORT 1883
#define MQTT_USERNAME    "d1"//用户名称和密码，用的公网可以不填可随机
#define MQTT_KEY         "d1"
 

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
 
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN 25

DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);
Adafruit_MQTT_Publish pub = Adafruit_MQTT_Publish(&mqtt,"my/mqtt/topic"); // 发布主题

DynamicJsonDocument doc(128);
char payload[128];

 
void setup() {
  dht.begin();//启动DHT11
  Serial.begin(115200);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
 
void MQTT_connect()//连接mqtt服务器，5s连接一次
{
  int8_t ret;
  if(mqtt.connected())
  {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 5;//尝试5次
  while((ret = mqtt.connect()) != 0)
  {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if(retries == 0)//如果五次都没连接上进入死循环
    {
      while(1);
    }
    Serial.println("MQTT Connected!");
  }
}
 
void loop() {
  doc["Humidity"] = dht.readHumidity();
  doc["Temperature"] = dht.readTemperature();
  if (isnan(doc["Humidity"]) || isnan(doc["Temperature"])) {
  Serial.println(F("Failed to read from DHT sensor!"));
  return;
  }
  MQTT_connect();
  Serial.print(F("\nSending val "));
  serializeJson(doc, payload);
  if(!pub.publish((uint8_t*)payload, strlen(payload)))//发布数据
  {
    Serial.println(F("Failed"));
  }else
  {
    Serial.println(F("OK"));
  }
  delay(2000);
}