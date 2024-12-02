/*
Pocket Radio Project
*/

/*
NOTE: README.txt not working, git process error, will be fixed later on
*/

#include <Arduino.h>
#include <TEA5767.h>
#include <Wire.h>
#include <SSD1306Wire.h>

#define FREQ A10 //Potentiometer Input
const byte buttons[] = {34,0,35};
#define BACK buttons[0];
#define HOME buttons[1];
#define NEXT buttons[2];
#define MIN 88.0 //Min Frequency
#define MAX 108.0 //Max Frequency
#define WIDTH 128
#define HEIGHT 64
#define MAX_FRAME_DELAY 128
#define TOUCH_THRESHOLD 25

TEA5767 radio = TEA5767();
SSD1306Wire lcd(0x3c, SDA, SCL);
int freq_read; //Potentiometer Input
float frequency = 100.7; //Radio Station
int SAMPLES = 10;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  radio.setFrequency(frequency);
  for(int i = 0;i < 3;i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  lcd.drawString(64, 10, String(frequency));
  lcd.clear();
  lcd.display();
  draw_main();
}
void loop() {
  freq_read = 0;
  for(int i = 0; i < SAMPLES; i++) {
    freq_read += analogRead(FREQ);
  }
  freq_read /= SAMPLES;
  frequency = map(freq_read, 0, 4095, MIN * 10, MAX * 10) / 10.0;
  //radio.setFrequency(frequency);   //Uncommented because the main circuit is not connected,just testing the lcd displau for now
  Serial.print(frequency);
  Serial.print(", Unfiltered: ");
  Serial.println(analogRead(FREQ));
  //Serial.print("Actual: ");
  //Serial.println(radio.getFrequency());
  delay(500);
} 

//draw the main menu screen
//Very rough idea of what it looks like for now, "Back", "Home", "Next" refer to 3 buttons on Makerboard B0, B1, B3
void draw_main() {
  lcd.clear();
  lcd.drawRect(0, 0, WIDTH, HEIGHT);
  lcd.drawString(10, 10, "Station: ");
  lcd.drawString(50, 10, String(frequency) + "FM");
  lcd.drawString(10, 20, "Song: ");
  lcd.drawString(50, 20, "Sample Title");
  lcd.drawString(10, 30, "Artist: ");
  lcd.drawString(50, 30, "Sample Artist");
  lcd.drawRect(10, HEIGHT - 20, 30, 15);
  lcd.drawString(12, HEIGHT - 20, "Back");
  lcd.drawRect(52, HEIGHT - 20, 30, 15);
  lcd.drawString(54, HEIGHT - 20, "Home");
  lcd.drawRect(95, HEIGHT - 20, 30, 15);
  lcd.drawString(97, HEIGHT - 20, "Next");
  lcd.display();
}


/*
Mechanical Updates:
Like 90% working, issue was the vcc cable to audio amplifier was not wired correctly
 12/1/24 2:19am
- Derek
*/