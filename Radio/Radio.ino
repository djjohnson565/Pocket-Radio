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
String stations[32][3] = {
  {"88.50", "WFCR", "Public"},
  {"89.30", "WAMH", "College"},
  {"90.30", "WAMC", "Public"},
  {"91.10", "WMUA", "UMass"},
  {"91.90", "WOZQ", "College"},
  {"92.30", "W222", "Oldies"},
  {"93.10", "WHYN", "Contemporary"},
  {"93.90", "WRSI", "Album"},
  {"94.30", "W232", "Hits"},
  {"94.70", "WMAS", "Holiday"},
  {"95.30", "WPVQ", "Country"},
  {"96.10", "WSRS", "Holiday"},
  {"96.50", "WTIC", "Contemporary"},
  {"96.90", "W245", "Oldies"},
  {"97.30", "WKXE", "Unknown"}, 
  {"97.70", "W249", "Hits"},
  {"98.30", "WHAI", "Contemporary"},
  {"98.90", "W255", "News"},
  {"99.30", "WLZX", "Rock"},
  {"99.90", "WKMY", "Christian"},
  {"100.70", "WZLX", "Rock"},
  {"100.90", "WRNX", "Country"},
  {"101.10", "WGIR", "Rock"},
  {"101.50", "W268C", "News"},
  {"102.10", "WAQY", "Rock"},
  {"103.30", "WXOJ", "Variety"},
  {"104.30", "DJKN", "Local"},
  {"104.90", "WYRY", "Country"},
  {"105.50", "WWEI", "Sports"},
  {"106.30", "WEIB", "Jazz"},
  {"106.90", "WCCC", "Christian"},
  {"107.70", "WAIY", "Religious"}
};            

int num_station = sizeof(stations) / sizeof(stations[0]);

void setFrequency(float frequency) {
  radio.setFrequency(frequency);
  rds_freq = int(frequency * 100);
  if(last_rds_freq != rds_freq) {
    radioInfo.clearRdsBuffer();
    fix_programInfo = "";
    fix_stationName = "";
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

void draw_main(float freq) {
  lcd.clear();
  lcd.setTextAlignment(TEXT_ALIGN_CENTER);
  //lcd.drawRect(0, 0, WIDTH, HEIGHT);
  String freq_string = String(freq) + "FM";
  bool found = false;
  int matchedStationIndex = -1;
  for (int i = 0; i < 31; ++i) {
    float stationFreq = stations[i][0].toFloat();
    if (abs(stationFreq - freq) < 0.1) {
      matchedStationIndex = i;
      found = true;
      break;
    }
  }
  
  if (found && matchedStationIndex != -1) {
    //String second_line = stations[matchedStationIndex][1] + " " + stations[matchedStationIndex][2];
    lcd.setFont(ArialMT_Plain_10);
    // lcd.drawString(2,23, second_line);
    lcd.drawString(64, 20, stations[matchedStationIndex][1]);
    //lcd.setFont(ArialMT_Plain_10);
    lcd.drawString(64, 32, stations[matchedStationIndex][2]);
  } else {
    lcd.setFont(ArialMT_Plain_10);
    lcd.drawString(64, 23, "Genre Unknown");
  }
  lcd.setFont(ArialMT_Plain_16);
  lcd.drawString(64, 2, freq_string);
  lcd.setFont(ArialMT_Plain_10);
  if(programInfo != NULL) {
    fix_programInfo = String(programInfo);
  }
  if(stationName != NULL) {
    fix_stationName = String(stationName);
  }
  
  if (fix_programInfo.length() > 15) {
    fix_programInfo = fix_programInfo.substring(0, 15);
  }else if(fix_programInfo.length() <= 0 || fix_programInfo == NULL) {
    fix_programInfo = "...";
  }
  if (fix_stationName.length() > 15) {
    fix_stationName = fix_stationName.substring(0, 15);
  }
  else if(fix_stationName.length() <= 0 || fix_stationName == NULL) {
    fix_stationName = "...";
  }
  lcd.drawString(64, 34, fix_stationName);
  lcd.drawString(64, 46, fix_programInfo);
  lcd.setTextAlignment(TEXT_ALIGN_RIGHT);
  lcd.setFont(ArialMT_Plain_16);
  if (!state) {
    lcd.drawString(127, 22, "K");
  } else {
    lcd.drawString(127, 22, "B");
    lcd.fillTriangle(120, 2, 115, 10, 125, 10);
    lcd.fillTriangle(120, 62, 115, 54, 125, 54);
  }
  lcd.setFont(ArialMT_Plain_10);
  lcd.display();
}
void draw_to_next(float now, float target) {
  Serial.println("Entering draw_to_next");
  Serial.print("Target value: ");
  Serial.println(target);
  if(now < target) {
    if (abs(target - stations[num_station - 1][0].toFloat()) < 0.1 && now <= stations[0][0].toFloat()) {
      for(float i = now; i >= 88.0; i-= 0.1) {
        draw_main(i);
        delay(200);
      }
      for (float i = 108.0; i >= target; i -= 0.1) {
        draw_main(i);
        delay(200);
      }
    }
    else {
      for (float i = now; i <= target; i += 0.1) {
        Serial.println("Entered the loop successfully");
        draw_main(i);
        delay(200);
      }
    }
  }
  else {
    if (abs(target - stations[0][0].toFloat()) < 0.1 && now >= stations[num_station - 1][0].toFloat()) {
      for(float i = now; i <= 108.0; i+= 0.1) {
        draw_main(i);
        delay(200);
      }
      for (float i = 88.0; i <= target; i += 0.1) {
        Serial.println("Entered the loop successfully");
        draw_main(i);
        delay(200);
      }
    }
    else {
      Serial.println("Entering draw_to_next_for_loop");
      for (float i = now; i >= target; i-= 0.1) {
        Serial.println("Entered the loop successfully");
        draw_main(i);
        delay(200);
      }
    }
  }
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
  //Find the index for the next station
  int station_index = 0;
  for (station_index; station_index < num_station && local2 > stations[station_index][0].toFloat(); station_index++) {
  }
  Serial.println("For loop not fail");
  Serial.print("Station index: ");
  Serial.println(station_index);
  if(up_pressed){
      if(station_index < num_station) {
        station_index++;
      }
      station_index %= num_station;
      Serial.println("Before drawing");
      draw_to_next(local, stations[station_index][0].toFloat());
      Serial.println("Finished Drawing");
      return stations[station_index][0].toFloat();
      // if(local >= 108.0) {
      //   local2 = 88.0;
      // }else {
      //   local2 += 0.1;
      // }
    }else if(down_pressed) {
      station_index--;
      if (station_index < 0) {
        station_index = num_station - 1;
      }
      Serial.println("Before drawing");
      draw_to_next(local, stations[station_index][0].toFloat());
      delay(200);
      Serial.println("Finished Drawing");
      return stations[station_index][0].toFloat();
      // if(local <= 88.0) {
      //   local2 = 108.0;
      // }else {
      //   local2 -= 0.1;
      // }
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
    pinMode(buttons[i], INPUT_PULLUP);
  }
  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  lcd.clear();
  lcd.display();
  lcd.setTextAlignment(TEXT_ALIGN_CENTER);
  draw_main(frequency);
  Serial.println("Setup Completed");
}
void loop() {
  currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    up_pressed = (digitalRead(UP) == LOW);
    down_pressed = (digitalRead(DOWN) == LOW);
    currentMillis_button = millis();
    curr_toggle_state = digitalRead(TOG);
    Serial.println("Assertion check 1");
    Serial.println("Check array length correctly?:");
    Serial.print(num_station);
    if(last_toggle_state == HIGH && curr_toggle_state == LOW) {
      state = !state;
    }
    Serial.println("Assertion check 2");
    last_toggle_state = curr_toggle_state;
    if(!state) {
      frequency = get_freq_pot();
      Serial.println("Assertion check 3.1");
    }else {
      frequency = get_button_read(frequency);
      Serial.println("Assertion check 3.2");
    }
    Serial.println("Setting Frequency");
    Serial.println("Assertion check 4");
    setFrequency(frequency);
    //Serial.println("Frequency Was Set");
    if ((millis() - status_elapsed) > MAX_DELAY_STATUS) {
      showStatus();
      status_elapsed = millis();
    }
    Serial.println("Assertion check 5");
    radioInfo.setRDS(true);
    checkRDS();
    Serial.println("Assertion check 6");
    if(!state) {
      Serial.print("Pot: ");
    }else {
      Serial.print("Button: ");
    }
    Serial.println("Assertion check 7");
    // Serial.print(frequency);
    // Serial.print(", Unfiltered: ");
    // Serial.println(analogRead(FREQ));
    draw_main(frequency);
    Serial.println("Assertion check 8");
  }
}
