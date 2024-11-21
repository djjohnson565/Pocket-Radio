/*
CODE NOT WRITTEN BY US, Used to test internet web scrapping.
*/
//Work In Progress [Non Functional]
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
String network_name;
String network_password;
String website;
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // set in STATION mode so we have Internet
  WiFi.begin(network_name, network_password);
  Serial.println("Connecting...");
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  HTTPClient http;
  http.begin(website); 
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
  }
  http.end();
}

void loop() {
}