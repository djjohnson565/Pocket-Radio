/*
Pocket Radio Project
*/

#include <Arduino.h>
#include <TEA5767.h>
#include <Wire.h>
#include <Radio.h>

TEA5767 radio = TEA5767();
float frequency = 99.3;//Radio Station

void setup() {
  Serial.begin(115200);
  Wire.begin();
  radio.setFrequency(frequency);
}

void loop() {
  Serial.println(frequency, radio.getVolume());
  Serial.println("commit 2");
} 