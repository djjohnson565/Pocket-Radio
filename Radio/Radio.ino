/*
Pocket Radio Project
*/
//INSTALL THE RADIO LIBRARY BY MARTHERTEL https://github.com/mathertel/Radio/archive/master.zip
#include <Arduino.h>
#include <TEA5767.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <radio.h>
#include <RDSParser.h>
#include <SI4703.h>

#define RESET_PIN 4
#define MODE_PIN 21

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

SI4703 radioInfo; //Si4703 Model for Radio information
RDSParser rds; //RDS handler

SSD1306Wire lcd(0x3c, SDA, SCL);
int freq_read; //Potentiometer Input
float frequency = 100.7; //Radio Station
int SAMPLES = 10;

void DisplayServiceName(const char *name) {
  Serial.print("RDS:");
  Serial.println(name);
  // lcd.print(name);
}  // DisplayServiceName()


/// Update the RDS Text
void DisplayRDSText(const char *txt) {
  Serial.print("Text:");
  Serial.println(txt);
  lcd.drawString(10, 20, "Text: ");
  lcd.drawString(50, 20, txt);
}  // DisplayRDSText()


void DisplayTime(uint8_t hour, uint8_t minute) {
  Serial.print("RDS-Time:");
  if (hour < 10) Serial.print('0');
  Serial.print(hour);
  Serial.print(':');
  if (minute < 10) Serial.print('0');
  Serial.println(minute);
}  // DisplayTime()


/// Display a label and value for several purpose
// void DisplayMenuValue(String label, int value) {
//   String out = label + ": " + value;

//   Serial.println(out);

//   lcd.setCursor(0, 1);
//   lcd.print(out + "  ");
// }

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

//There is not Song, Artist sections because I don't know the format of the txt string extracted from RDS. After testing and seeing I can format it again.
//The thing drew inplace of those sections is the full raw text extract from RDS (see the displayRDSText), i think
void draw_real() {
  lcd.clear();
  lcd.drawRect(0, 0, WIDTH, HEIGHT);
  lcd.drawString(10, 10, "Station: ");
  lcd.drawString(50, 10, String(frequency) + "FM");
  lcd.drawRect(10, HEIGHT - 20, 30, 15);
  lcd.drawString(12, HEIGHT - 20, "Back");
  lcd.drawRect(52, HEIGHT - 20, 30, 15);
  lcd.drawString(54, HEIGHT - 20, "Home");
  lcd.drawRect(95, HEIGHT - 20, 30, 15);
  lcd.drawString(97, HEIGHT - 20, "Next");
  lcd.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  radio.setFrequency(frequency);
  radioInfo.setBandFrequency(RADIO_BAND_FM, int(frequency*100));
  radioInfo.setMute(true);

  //Setup for the Si4703
  radioInfo.setup(RADIO_RESETPIN, RESET_PIN);
  radioInfo.setup(RADIO_MODEPIN, MODE_PIN);

  // Enable information to the Serial port
  radioInfo.debugEnable(true);
  radioInfo._wireDebug(true);

  radioInfo.setup(RADIO_FMSPACING, RADIO_FMSPACING_200);   // for US
  radioInfo.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_75);  // for US

  radioInfo.initWire(Wire);

  radioInfo.setBandFrequency(FIX_BAND, 10070);
  radioInfo.setVolume(8);
  radioInfo.setMono(false);
  radioInfo.setMute(false);

//Adding a function to proccess the RDS data to the radio module when it received RDS data
  radioInfo.attachReceiveRDS(RDS_process);
//Adding functions to do when the RDS data is processed 
  rds.attachServiceNameCallback(DisplayServiceName); //Service Provider
  rds.attachTextCallback(DisplayRDSText);  //The full RDS data (don't know the format yet)
//Tell how long we are playing the radio?
  rds.attachTimeCallback(DisplayTime);

  for(int i = 0;i < 3;i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  // lcd.drawString(64, 10, String(frequency));
  // lcd.clear();
  // lcd.display();
  draw_main();
}
void loop() {
  char rdsData[8];
  radioInfo.formatFrequency(rdsData, sizeof(rdsData)); //format the frequency for printing, ALTERNATIVE OPTION
  freq_read = 0;
  for(int i = 0; i < SAMPLES; i++) {
    freq_read += analogRead(FREQ);
  }
  freq_read /= SAMPLES;
  frequency = map(freq_read, 0, 4095, MIN * 10, MAX * 10) / 10.0;
  radio.setFrequency(frequency);   //Uncommented because the main circuit is not connected,just testing the lcd displau for now
  radioInfo.setFrequency(map(freq_read, 0, 4095, MIN * 100, MAX * 100)); //Set the frequency to be the same as the TEA5767, different format
  Serial.print(frequency);
  Serial.print(", Unfiltered: ");
  Serial.println(analogRead(FREQ));

  radioInfo.checkRDS(); //Check if the RDS data is good
  RADIO_INFO info;
  radioInfo.getRadioInfo(&info);
  if (!info.rds) {
    draw_main();
  } //Check if the rds is there or not if not draw the default one with no info
  else {
    draw_real();
  }
  delay(500);
} 

