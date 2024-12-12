/*
Pocket Radio Project
*/
#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <TEA5767.h>
#include <SI4703.h>
#include <SSD1306Wire.h>

//Function Buttons
const byte buttons[] = {34,0,35};
const int UP = buttons[0];
const int TOG = buttons[1];
const int DOWN = buttons[2];

#define FREQ A10 //Potentiometer Input
#define MIN 88.0 //Min Frequency
#define MAX 108.0 //Max Frequency

//Screen Dimensions
#define WIDTH 128
#define HEIGHT 64

//Initialize Radios
TEA5767 radio = TEA5767();
SI4703 radioInfo = SI4703();

//Initialize LCD
SSD1306Wire lcd(0x3c, SDA, SCL);

int freq_read; //potentiometer reading
float frequency = 88.0;
bool up_pressed;
bool toggle_pressed;
bool down_pressed;
bool last_toggle_state = HIGH;
bool curr_toggle_state = HIGH;
bool state;

//Non-blocking delay (main loop)
unsigned long previousMillis = 0;
const long interval = 500;
unsigned long currentMillis = millis();

//Non-blocking delay (buttons)
unsigned long previousMillis_button = 0;
const long interval_button = 100;
unsigned long currentMillis_button = millis();

struct RadioStation {
  String frequency;
  String callSign;
  String format;
};

//local Amherst, MA stations: https://radio-locator.com/cgi-bin/locate?select=city&city=Amherst&state=MA
RadioStation stations[] = {
  {"88.5", "WFCR", "Public"},
  {"89.3", "WAMH", "College"},
  {"90.3", "WAMC", "Public"},
  {"91.1", "WMUA", "UMass"},
  {"91.9", "WOZQ", "College"},
  {"92.3", "W222CH", "Oldies"},
  {"93.1", "WHYN", "Cont."},
  {"93.9", "WRSI", "Album"},
  {"94.3", "W232BW", "Hits"},
  {"94.7", "WMAS", "Holiday"},
  {"95.3", "WPVQ", "Country"},
  {"96.1", "WSRS", "Holiday"},
  {"96.5", "WTIC", "Cont."},
  {"96.9", "W245BK", "Oldies"},
  {"97.3", "WKXE", "???"},
  {"97.7", "W249DP", "Hits"},
  {"98.3", "WHAI", "Cont."},
  {"98.9", "W255DL", "News"},
  {"99.3", "WLZX", "Rock"},
  {"99.9", "WKMY", "Christian"},
  {"100.7", "WZLX", "Rock"},
  {"100.9", "WRNX", "Country"},
  {"101.1", "WGIR", "Rock"},
  {"101.5", "W268CZ", "News"},
  {"102.1", "WAQY", "Rock"},
  {"103.3", "WXOJ", "Variety"},
  {"104.9", "WYRY", "Country"},
  {"105.5", "WWEI", "Sports"},
  {"106.3", "WEIB", "Jazz"},
  {"106.9", "WCCC", "Christian"},
  {"107.7", "WAIY", "Religious"}
};

//This needs a new layout once the RDS info format has been found
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
  lcd.drawString(12, HEIGHT - 20, "Down");
  lcd.drawRect(52, HEIGHT - 20, 30, 15);
  lcd.drawString(54, HEIGHT - 20, "Tog");
  lcd.drawRect(95, HEIGHT - 20, 30, 15);
  lcd.drawString(97, HEIGHT - 20, "Up");
  if(!state) {
    lcd.drawString(10, 1, "Knob");
  }else {
    lcd.drawString(10, 1, "Buttons");
  }
  lcd.display();
}

float get_freq_pot() {
  freq_read = 0;
  for(int i = 0; i < 10; i++) {
    freq_read += analogRead(FREQ);
  }
  freq_read /= 10;
  return map(freq_read, 0, 4095, MIN * 10, MAX * 10) / 10.0;
}

void get_button_read() {
  if(up_pressed) {
      if(frequency >= 108.0) {
        frequency = 88.0;
      }else {
        frequency += 0.1;
      }
      delay(200);
    }else if(down_pressed) {
      if(frequency <= 88.0) {
        frequency = 108.0;
      }else {
        frequency -= 0.1;
      }
      delay(200);
    }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  //radioInfo.init();

  radio.setFrequency(frequency);
  //radioInfo.setBandFrequency(RADIO_BAND_FM, int(frequency*10));
  //radioInfo.setMute(true);

  for(int i = 0;i < 3;i++) {
    pinMode(buttons[i], INPUT);
  }

  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  lcd.clear();
  lcd.display();
  draw_main();
}
void loop() {
  currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    up_pressed = (digitalRead(UP) == LOW);
    toggle_pressed = (digitalRead(TOG) == LOW);
    down_pressed = (digitalRead(DOWN) == LOW);
    currentMillis_button = millis();
    curr_toggle_state = digitalRead(TOG);
    if(last_toggle_state == HIGH && curr_toggle_state == LOW) {
      state = !state;
    }
    last_toggle_state = curr_toggle_state;
    if(!state) { //default pot reading
      frequency = get_freq_pot();
    }else { //channel seeker
      get_button_read();
    }
    radio.setFrequency(frequency);
    //radioInfo.setBandFrequency(RADIO_BAND_FM, int(frequency*10));
    if(!state) {
      Serial.print("Pot: ");
    }else {
      Serial.print("Button: ");
    }
    Serial.print(frequency);
    Serial.print(", Unfiltered: ");
    Serial.println(analogRead(FREQ));
    draw_main();
  }
}