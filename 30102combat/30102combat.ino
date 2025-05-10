#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <Wire.h>


MAX30105 particleSensor;

// 新增血氧检测变量（严格保持原有变量不变）
#define MAX_BRIGHTNESS 255
uint32_t irBuffer[100];      // 红外数据缓冲区
uint32_t redBuffer[100];     // 红光数据缓冲区
int32_t bufferLength = 100;  // 缓冲区长度
int32_t spo2;                // 血氧值
int8_t validSPO2;            // 血氧有效性标志
int32_t heartRate;           // 心率值（算法计算）
int8_t validHeartRate;       // 心率有效性标志

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))  // 默认使用I2C，400KHZ频率
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                     //使用默认设置配置传感器
  particleSensor.setPulseAmplitudeRed(0x0A);  // 将红色LED拉低，表示传感器正在运行

  // 新增：初始化血氧检测缓冲区
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false)
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
}

void loop() {
  long irValue = particleSensor.getIR();

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

  // 输出结果

  if (irValue < 50000 || validSPO2 == 0 || validHeartRate == 0) {
    Serial.print("heartRate=---, SpO2=---");
  } else {
      Serial.print(", heartRate=");
      Serial.print(heartRate);
      Serial.print(", SpO2=");
      Serial.print(spo2);
      Serial.print("%");
  }

  Serial.println();
}