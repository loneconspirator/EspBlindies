/**
 * ESP8266 Wireless Blindie Driver
 * Mike McCracken
 * 
 * Drives blindie lights on ESP8266 from UDP commands
 */

#include "Blindy.h"
#include "BlindyRGB.h"
#include <WifiCreds.h>
#include "WifiDefaults.h"
#include <EEPROM.h>
#include <stdio.h>

//#include "espconn.h" // I think this is supposed to help me discover if the packet I get was broadcast or straight to me

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

WifiCreds wifi(DEFAULT_SSID, DEFAULT_PASSWORD);

int pwm_map[256];
unsigned int delay_map[256];

unsigned int localPort = 2390;
char packetBuffer[255]; //buffer to hold incoming packet
WiFiUDP udp;

// This is the pin the Big LED Strip will be attached to
#define LED_PIN 12
// This is the pin of the blue LED on the huzzah which is for some reason backwards
#define LEDIND_PIN 2
// onboard red pin, also backwards
#define IND_PIN 0
// Output pin for the addressible LEDs
#define RGB_PIN 14
#define NUM_RGBS 60

#define BUTTON_PIN 4

/////// CONSIDER TRYING FINER GRANULARITY FOR DIMMER VALUES
#define PWM_RANGE 1023

#define RANDOM_RANGE 40

bool first_pass = true;

Blindy *curBlindy = NULL;
BlindyRGB *curRgb = NULL;

void setupLedPins() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LEDIND_PIN, OUTPUT);
  analogWriteRange(PWM_RANGE);

  // Make the fade curve appear smoother
  for (int i=0; i<256; i++) {
    double x = (double) i; double step1 = x/2.55; double step2 = step1 + 16.0;
    double step3 = step2 / 116.0; double step4 = pow(step3, 3); double step5 = step4 * (double) PWM_RANGE;
    pwm_map[i] = (int) step5;
    if (i < 3) pwm_map[i] = i;
  }
}

// This sets the level by 
void setLedLevel(unsigned char level) {
  int pwm_level = pwm_map[level];
  analogWrite(LED_PIN, pwm_level);
  analogWrite(LEDIND_PIN, PWM_RANGE - pwm_level);
}

void button() {
  if (wifi.is_wifi_enabled() && !wifi.is_connected())
    wifi.disable_wifi();
}

char * my_name(){
  return wifi.read_variable(130, 15, "<no_name>\0");
}
void my_name(char *new_name){
  wifi.write_variable(130, 15, new_name);
}
int num_rgbs(){
  return (int)wifi.read_variable(128, 1, "\0")[0];
}
void num_rgbs(int num){
  char _num = (char) num;
  wifi.write_variable(128, 1, &_num);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize pins
  setupLedPins();
  setLedLevel(0);

  pinMode(IND_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // This line below was getting my ESP into an unhappy state
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button, FALLING);

  pinMode(RGB_PIN, OUTPUT);

  for (int i=0; i<256; i++)
    delay_map[i] = (unsigned int) (255-i)*40;

  Serial.begin(115200); while(!Serial) delayMicroseconds(10); // Open Serial connection and wait for it to be good

  delay(500);
  Serial.println();
  Serial.print("Starting up Blindies version: "); Serial.println(Blindy::version);

  wifi.connect(IND_PIN);

  if (wifi.is_wifi_enabled()) {
    udp.begin(localPort);
    
    unsigned int seed = WiFi.localIP()[3];
    Serial.print("Seeding with: "); Serial.println(seed);
    Blindy::seed_random(seed);
  }

//  Blindy::set_mic_pin(A0);
  BlindyRGB::initRGB(num_rgbs(), RGB_PIN);
  Serial.print("Setting up "); Serial.print(num_rgbs()); Serial.println(" RGB lights"); Serial.flush();

  packetBuffer[0] = '8'; packetBuffer[1] = 255;  packetBuffer[2] = '\0'; 
  packetBuffer[0] = '0'; packetBuffer[1] = '\0';
  curBlindy = Blindy::new_command(packetBuffer, NULL);
  //packetBuffer[0] = 'A'; packetBuffer[1] = '\0';
  strcpy(packetBuffer, "D\0\0");
  curRgb = BlindyRGB::new_command(packetBuffer, NULL);

  Serial.println((int) curRgb); Serial.flush(); // This was just in there to make sure it was getting assigned

  setLedLevel(curBlindy->new_brightness());
  if(curRgb) curRgb->new_brightness();
}

unsigned long lastMillis = 0;

void loop() {
  unsigned long curMillis = millis();

  if (wifi.is_wifi_enabled()) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
//      Serial.print("Received packet length "); Serial.println(packetSize); 
//      Serial.print("From "); IPAddress remoteIp = udp.remoteIP(); Serial.print(remoteIp);
//      Serial.print(", port "); Serial.println(udp.remotePort());

      // read the packet into packetBufffer
      int len = udp.read(packetBuffer, 255);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      Serial.print("Just got:"); Serial.println(packetBuffer);
      curBlindy = Blindy::new_command(packetBuffer, curBlindy);
      curRgb = BlindyRGB::new_command(packetBuffer, curRgb);
      // If it's a roll call, I've got to handle it
      if (packetBuffer[0] == Blindy::roll_call_code) {
        int max_pack_len = 72;
        char *id = new char[max_pack_len];
        char *num = new char[10];
        itoa(WiFi.localIP()[0], num, 10);
        strcpy(id, num);
        strcat(id, ".");
        itoa(WiFi.localIP()[1], num, 10);
        strcat(id, num);
        strcat(id, ".");
        itoa(WiFi.localIP()[2], num, 10);
        strcat(id, num);
        strcat(id, ".");
        itoa(WiFi.localIP()[3], num, 10);
        strcat(id, num);
        strcat(id, "-");
        strcat(id, Blindy::version);
        strcat(id, "-");
        strcat(id, my_name());
        strcat(id, "-");
        itoa(num_rgbs(), num, 10);
        strcat(id, num);
        strcat(id, "-");
        strcat(id, wifi.mac_id());
        udp.beginPacket(udp.remoteIP(), localPort);//udp.remotePort());
        udp.write(id, max_pack_len);
        udp.endPacket();
        Serial.print("Sent: ");Serial.print(id);Serial.print(" to ");Serial.print(udp.remoteIP());Serial.print(":");Serial.println(udp.remotePort());
        delete id;
        delete num;
      }
      // set the number of RGBs
      if (packetBuffer[0] == '+') {
        num_rgbs(packetBuffer[1]);
        udp.beginPacket(udp.remoteIP(), localPort);//udp.remotePort());
        udp.write("Set # of RGBs - must restart unit", 34);
        udp.endPacket();
      }
      // set the unit name
      if (packetBuffer[0] == '!') {
        my_name(packetBuffer+1);
        udp.beginPacket(udp.remoteIP(), localPort);//udp.remotePort());
        udp.write("Set my name", 34);
        udp.endPacket();
      }
    }
  }
  // Do the adjustment to the light level according to the variables
//  Serial.printf("Millis since last pass: %d\n", curMillis - lastMillis);
//  lastMillis = curMillis;
  if (first_pass) {
    if (curRgb)
      curRgb->reset_next_action();
    first_pass = false;
  }
  if (curBlindy->is_time_to_act()) {
    unsigned char newLevel = curBlindy->new_brightness();
//    Serial.print("Setting level to: "); Serial.println(newLevel);
    setLedLevel(newLevel);
  }
  if (curRgb && curRgb->is_time_to_act()) {
    curRgb->new_brightness();
  }
  delayMicroseconds(450);
}
