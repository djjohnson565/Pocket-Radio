/*
Pocket Radio Project
*/
#include <Arduino.h>
#include <TEA5767.h>
#include <Wire.h>
//#include <Radio.h>

#define FREQ A10 //Potentiometer Input
#define MIN 88.0 //Min Frequency
#define MAX 108.0 //Max Frequency

TEA5767 radio = TEA5767();
int freq_read; //Potentiometer Input
float frequency; //Radio Station
int SAMPLES = 10;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  radio.setFrequency(frequency);
}
void loop() {
  freq_read = 0;
  for(int i = 0; i < SAMPLES; i++) {
    freq_read += analogRead(FREQ);
  }
  freq_read /= SAMPLES;
  frequency = map(freq_read, 0, 4095, MIN * 10, MAX * 10) / 10.0;
  radio.setFrequency(frequency);
  Serial.print(frequency);
  Serial.print(", Unfiltered: ");
  Serial.println(analogRead(FREQ));
  //Serial.print("Actual: ");
  //Serial.println(radio.getFrequency());
  delay(500);
} 

/*
Mechanical Updates:
Like 90% working, issue was the vcc cable to audio amplifier was not wired correctly
 12/1/24 2:19am
- Derek
*/