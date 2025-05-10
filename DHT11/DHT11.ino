/*
DHT11温湿度传感器接线
DHT11电源 3.3V
数据接引脚D4
*/
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
 
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN 25
DHT dht(DHTPIN, DHTTYPE);
void setup() {
  Serial.begin(115200);
  dht.begin();
}
 
void loop() {
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print("湿度为：");
  Serial.println(h);
  Serial.print("温度为：");
  Serial.println(t);
  delay(3000);
}
 
 
 