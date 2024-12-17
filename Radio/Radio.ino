#include <Arduino.h>
#include <Wire.h>
#include <TEA5767.h>
#include <SI470X.h>
#include <SSD1306Wire.h>

//Function Buttons
const byte buttons[] = {34,0,35};
const int UP = buttons[0];
const int TOG = buttons[1];
const int DOWN = buttons[2];

#define FREQ A10 //Potentiometer Input
#define MAX_DELAY_RDS 40 //40ms polling
#define MAX_DELAY_STATUS 2000
long status_elapsed = millis();
#define MIN 88.0 //Min Frequency
#define MAX 108.0 //Max Frequency

//Screen Dimensions
#define WIDTH 128
#define HEIGHT 64

//Initialize Radios
SI470X radioInfo;
TEA5767 radio = TEA5767();

//Initialize LCD
SSD1306Wire lcd(0x3c, SDA, SCL);

int freq_read; //potentiometer reading
float frequency = 88.0;
float lastFrequency = frequency;
int rds_freq = int(frequency * 100);
int last_rds_freq = rds_freq;
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

//RDS Values
char *programInfo;
char *stationName;
char *rdsTime;
String fix_programInfo;
String fix_stationName;

//local Amherst, MA stations: https://radio-locator.com/cgi-bin/locate?select=city&city=Amherst&state=MA
String stations[31][3] = {
  {"88.50", "WFCR", "Public"},
  {"89.30", "WAMH", "College"},
  {"90.30", "WAMC", "Public"},
  {"91.10", "WMUA", "UMass"},
  {"91.90", "WOZQ", "College"},
  {"92.30", "W222CH", "Oldies"},
  {"93.10", "WHYN", "Contemporary"},
  {"93.90", "WRSI", "Album"},
  {"94.30", "W232BW", "Hits"},
  {"94.70", "WMAS", "Holiday"},
  {"95.30", "WPVQ", "Country"},
  {"96.10", "WSRS", "Holiday"},
  {"96.50", "WTIC", "Contemporary"},
  {"96.90", "W245BK", "Oldies"},
  {"97.30", "WKXE", "Unknown"}, 
  {"97.70", "W249DP", "Hits"},
  {"98.30", "WHAI", "Contemporary"},
  {"98.90", "W255DL", "News"},
  {"99.30", "WLZX", "Rock"},
  {"99.90", "WKMY", "Christian"},
  {"100.70", "WZLX", "Rock"},
  {"100.90", "WRNX", "Country"},
  {"101.10", "WGIR", "Rock"},
  {"101.50", "W268CZ", "News"},
  {"102.10", "WAQY", "Rock"},
  {"103.30", "WXOJ", "Variety"},
  {"104.90", "WYRY", "Country"},
  {"105.50", "WWEI", "Sports"},
  {"106.30", "WEIB", "Jazz"},
  {"106.90", "WCCC", "Christian"},
  {"107.70", "WAIY", "Religious"}
};                       

void setFrequency(float frequency) {
  radio.setFrequency(frequency);
  rds_freq = int(frequency * 100);
  if(last_rds_freq != rds_freq) {
    radioInfo.clearRdsBuffer();
    Wire.end();
    Wire.begin();
    radioInfo.setup(15, SDA);
    delay(500);
    radioInfo.setRDS(true);
    radioInfo.setFrequency(rds_freq);
    delay(500);
    checkRDS();
    Serial.print("RDS Frequency: ");
    Serial.println(rds_freq);
    last_rds_freq = int(frequency * 100);
  }
}

void draw_main() {
  lcd.clear();
  lcd.drawRect(0, 0, WIDTH, HEIGHT);
  String freq_string = String(frequency);
  bool found = false;
  int matchedStationIndex = -1;
  for (int i = 0; i < 31; ++i) {
    float stationFreq = stations[i][0].toFloat();
    if (abs(stationFreq - frequency) < 0.1) {
      matchedStationIndex = i;
      found = true;
      break;
    }
  }
  if (found && matchedStationIndex != -1) {
    lcd.drawString(10, 10, freq_string + "FM " + stations[matchedStationIndex][1]);
    lcd.drawString(10, 20, stations[matchedStationIndex][2]);
  } else {
    lcd.drawString(10, 10, freq_string + "FM");
    lcd.drawString(10, 20, "Genre Unknown");
  }
  if(programInfo != NULL) {
    fix_programInfo = String(programInfo);
  }
  if(stationName != NULL) {
    fix_stationName = String(stationName);
  }
  if (fix_programInfo.length() > 20 || fix_programInfo == NULL) {
    fix_programInfo = fix_programInfo.substring(0, 20);
  }else if(fix_programInfo.length() <= 0 || fix_stationName == NULL) {
    fix_programInfo = "...";
  }
  if (fix_stationName.length() > 20) {
    fix_stationName = fix_stationName.substring(0, 20);
  }
  else if(fix_stationName.length() <= 0) {
    fix_stationName = "...";
  }
  lcd.drawString(10, 30, fix_stationName);
  lcd.drawString(10, 40, fix_programInfo);
  if (!state) {
    lcd.drawString(10, 1, "Knob");
  } else {
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

float get_button_read(float local) {
  float local2 = local;
  if(up_pressed) {
      if(local >= 108.0) {
        local2 = 88.0;
      }else {
        local2 += 0.1;
      }
      delay(200);
    }else if(down_pressed) {
      if(local <= 88.0) {
        local2 = 108.0;
      }else {
        local2 -= 0.1;
      }
      delay(200);
    }
  return local2;
}

//Written by https://github.com/pu2clr/SI470X/blob/master/examples/si470x_01_serial_monitor/si470x_01_RDS/si470x_01_RDS.ino
bool checkI2C() {
  byte error, address;
  int nDevices;
  Serial.println("I2C bus Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("\nI2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("\nUnknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    return false;
  }
  else {
    Serial.println("done\n");
    return true;
  }
}

//Written by https://github.com/pu2clr/SI470X/blob/master/examples/si470x_01_serial_monitor/si470x_01_RDS/si470x_01_RDS.ino
void showStatus() {
  char aux[80];
  sprintf(aux, "\nYou are tuned on %u MHz | RSSI: %3.3u dbUv | Vol: %2.2u | Stereo: %s\n", radioInfo.getFrequency(), radioInfo.getRssi(), radioInfo.getVolume(), (radioInfo.isStereo()) ? "Yes" : "No");
  Serial.print(aux);
  status_elapsed = millis();
}

//Written by https://github.com/pu2clr/SI470X/blob/master/examples/si470x_01_serial_monitor/si470x_01_RDS/si470x_01_RDS.ino
void showRdsData() {
  if (programInfo) {
    Serial.print("\nProgram Info...: ");
    Serial.println(programInfo);
  }

  if (stationName) {
    Serial.print("\nStation Name...: ");
    Serial.println(stationName);
  }

  if (rdsTime) {
    Serial.print("\nUTC / Time....: ");
    Serial.println(rdsTime);
  }
}

//Written by https://github.com/pu2clr/SI470X/blob/master/examples/si470x_01_serial_monitor/si470x_01_RDS/si470x_01_RDS.ino
void checkRDS() {
  if (radioInfo.getRdsReady()) {
    programInfo = radioInfo.getRdsProgramInformation();
    stationName = radioInfo.getRdsStationName();
    rdsTime = radioInfo.getRdsTime();
    showRdsData();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!checkI2C())
  {
      Serial.println("\nCheck your circuit!");
      while(1);
  }
  radioInfo.setup(15, SDA);
  delay(500);
  radioInfo.setRDS(true);

  for(int i = 0;i < 3;i++) {
    pinMode(buttons[i], INPUT);
  }
  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  lcd.clear();
  lcd.display();
  draw_main();
  Serial.println("Setup Completed");
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
    if(!state) {
      frequency = get_freq_pot();
    }else {
      frequency = get_button_read(frequency);
    }
    Serial.println("Setting Frequency");
    setFrequency(frequency);
    Serial.println("Frequency Was Set");
    if ((millis() - status_elapsed) > MAX_DELAY_STATUS) {
      showStatus();
      status_elapsed = millis();
    }
    radioInfo.setRDS(true);
    checkRDS();
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