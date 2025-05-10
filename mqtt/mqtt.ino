#include <WiFi.h>
#include "WiFiClient.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <Adafruit_MPU6050.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>

#define WLAN_SSID "WIFI6-2.4G"  //wifi名称
#define WLAN_PASS "200146dd"    //wifi密码

#define MQTT_SERVER "broker.emqx.io"  //服务器地址，这里使用的是公共服务器进行转发
#define MQTT_SERVERPORT 1883          //服务器端口
#define MQTT_USERNAME "D1"            //用户名称和密码
#define MQTT_KEY "D1"

#define DHTTYPE DHT11  
#define DHTPIN 19      //DHT11引脚

#define MAX_BRIGHTNESS 255

Adafruit_MPU6050 mpu;
float G = 9.8;
sensors_event_t a, g, temp;

MAX30105 particleSensor;
uint32_t irBuffer[100];      // 红外数据缓冲区
uint32_t redBuffer[100];     // 红光数据缓冲区
int32_t bufferLength = 100;  // 缓冲区长度
int32_t spo2;                // 血氧值
int8_t validSPO2;            // 血氧有效性标志
int32_t heartRate;           // 心率值（算法计算）
int8_t validHeartRate;       // 心率有效性标志
long irValue;

DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);
Adafruit_MQTT_Publish pub = Adafruit_MQTT_Publish(&mqtt, "july/mqtt/topic");  // 发布主题

DynamicJsonDocument doc(128);
char payload[256];

void setup() {
  dht.begin();  //启动DHT11
  Serial.begin(115200);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(1000);
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  mpu.begin();
  // set accelerometer range to +-8G
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // set gyro range to +- 500 deg/s
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  // set filter bandwidth to 21 Hz
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);


  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))  // 默认使用I2C，400KHZ频率
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                     //使用默认设置配置传感器
  particleSensor.setPulseAmplitudeRed(0x0A);  // 将红色LED拉低，表示传感器正在运行

  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false)
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
}

void MQTT_connect()  //连接mqtt服务器，5s连接一次
{
  int8_t ret;
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 5;  //尝试5次
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0)  //如果五次都没连接上进入死循环
    {
      while (1)
        ;
    }
    Serial.println("MQTT Connected!");
  }
}

void loop() {
  //doc["Humidity"] = dht.readHumidity();
  mpu.getEvent(&a, &g, &temp);

  irValue = particleSensor.getIR();

  // 新增血氧检测逻辑（不影响原有心率检测）
  // 滑动更新缓冲区
  for (byte i = 25; i < 100; i++) {
    redBuffer[i - 25] = redBuffer[i];
    irBuffer[i - 25] = irBuffer[i];
  }

  // 填充新数据
  for (byte i = 75; i < 100; i++) {
    while (particleSensor.available() == false)
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // 计算心率血氧值
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  if (irValue < 50000 || validSPO2 == 0 || validHeartRate == 0 || heartrate > 150 || heartrate < 40) {//处理异常值
    doc["Temperature"] = dht.readTemperature();
    doc["Oxygen"] = 0.0;
    doc["heartrate"] = 0.0;
    doc["x"] = a.acceleration.x / G;
    doc["y"] = a.acceleration.y / G;
    doc["z"] = a.acceleration.z / G;
  } else {
    doc["Temperature"] = dht.readTemperature();
    doc["Oxygen"] = spo2;
    doc["heartrate"] = heartRate;
    doc["x"] = a.acceleration.x / G;
    doc["y"] = a.acceleration.y / G;
    doc["z"] = a.acceleration.z / G;
  }

  if (isnan(doc["Temperature"] || doc["Oxygen"] || doc["heartrate"] || doc["acceleration"] || doc["x"] || doc["y"] || doc["z"])) {
    Serial.println(F("Failed to read from sensor!"));
    return;
  }
  MQTT_connect();
  serializeJson(doc, payload);
  if (!pub.publish((uint8_t*)payload, strlen(payload)))  //发布数据
  {
    Serial.println(F("Failed"));
  } else {
    Serial.println(payload);
  }
  delay(500);
}