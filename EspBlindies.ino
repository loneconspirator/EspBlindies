/**
 * ESP8266 Wireless Blindie Driver
 * Mike McCracken
 * 
 * Drives blindie lights on ESP8266 from UDP commands
 */

#include "Blindy.h"
#include <WifiCreds.h>
#include "WifiDefaults.h"
//#include "espconn.h" // I think this is supposed to help me discover if the packet I get was broadcast or straight to me

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#include <EEPROM.h>

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

#define BUTTON_PIN 4

/////// CONSIDER TRYING FINER GRANULARITY FOR DIMMER VALUES
#define PWM_RANGE 1023

#define RANDOM_RANGE 40

Blindy *curBlindy;

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

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize pins
  setupLedPins();
  setLedLevel(0);

  pinMode(IND_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button, FALLING);

  for (int i=0; i<256; i++)
    delay_map[i] = (unsigned int) (255-i)*40;

  Serial.begin(115200); while(!Serial) ; // Open Serial connection and wait for it to be good

  wifi.connect(IND_PIN);

  if (wifi.is_wifi_enabled()) {
    udp.begin(localPort);
    
    unsigned int seed = WiFi.localIP()[3];
    Serial.print("Seeding with: "); Serial.println(seed);
    Blindy::seed_random(seed);
  }

  Blindy::set_mic_pin(A0);

//  packetBuffer[0] = '8'; packetBuffer[1] = 255;  packetBuffer[2] = '\0'; 
  packetBuffer[0] = '0'; packetBuffer[1] = '\0';
  curBlindy = Blindy::new_command(packetBuffer, NULL);

  setLedLevel(curBlindy->new_brightness());
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
      // If it's a roll call, I've got to handle it
      if (packetBuffer[0] = Blindy::roll_call_code) {
        char *id = new char[35];
        strcpy(id, wifi.mac_id());
        strcat(id, "-");
        strcat(id, Blindy::version);
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write(wifi.mac_id(), 19);
        udp.endPacket();
        delete id;
      }
    }
  }
  // Do the adjustment to the light level according to the variables
//  Serial.printf("Millis since last pass: %d\n", curMillis - lastMillis);
//  lastMillis = curMillis;
  if (curBlindy->is_time_to_act()) {
    unsigned char newLevel = curBlindy->new_brightness();
//    Serial.print("Setting level to: "); Serial.println(newLevel);
    setLedLevel(newLevel);
  }
  delayMicroseconds(450);
}
