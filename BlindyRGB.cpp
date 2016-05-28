#include "BlindyRGB.h"

int BlindyRGB::_num_leds = 0;
uint16_t * BlindyRGB::leds = NULL;
Adafruit_NeoPixel * BlindyRGB::strip = NULL;

#define NUM_PULSES 21
uint16_t pulse[NUM_PULSES] = {
  0x000011, 0x000022, 0x000044, 0x000077, 0x0000AA, 0x0000EE, 0x0000FF,
  0x1111FF, 0x2222FF, 0x6666FF, 0xBBBBFF, 0xFFFFFF, 0x8888FF, 0x2222FF,
  0x0000FF, 0x0000EE, 0x0000AA, 0x000077, 0x000044, 0x000022, 0x000000};

#define EYE_SIZE 4
uint16_t cylon_eye[EYE_SIZE] = {
  0xFFFFFF, 0x8888FF, 0x333388, 0x000033
};

void BlindyRGB::clear_values(){
  for(int i=0; i<_num_leds; i++)
    leds[i] = 0x000000;
}

void BlindyRGB::write_rgbs(){
//  Serial.print("w");
  for(int i=0; i<_num_leds; i++){
    strip->setPixelColor(i, leds[i]);
//    Serial.print(BlindyRGB::leds[i]); Serial.print(" ");
  }
//  Serial.println(); Serial.flush();
  strip->show();
}


void BlindyRGB::initRGB(int num_leds, int rgb_pin) {
  if (num_leds != _num_leds) {
    _num_leds = 0;
    if (leds != NULL) {
      delete[] leds; leds = NULL;
      delete strip; strip = NULL;
    }
    if (num_leds > 0) {
      leds = new uint16_t[num_leds];
      strip = new Adafruit_NeoPixel(num_leds, rgb_pin, NEO_GRB + NEO_KHZ800); 
      strip->begin();
      strip->show();
      // Serial.print("Just allocated an array for LEDs of size: "); Serial.println(num_leds);
      _num_leds = num_leds;
    };
  }
  clear_values();
}

BlindyRGB *BlindyRGB::new_command(char * args, BlindyRGB *previous) {
  BlindyRGB *new_mode = NULL;
  unsigned char cur_level = 0;
  // Serial.print("1) new_mode: "); Serial.println((int) new_mode); Serial.flush();
  if (previous) {
    new_mode = previous->next_command(args);
    cur_level = previous->cur_level();
  }
  // Serial.print("2) new_mode: "); Serial.println((int) new_mode); Serial.flush();
  if (!new_mode) {
    // Serial.println("2) !new_mode"); Serial.flush();
    new_mode = new_mode_from_scratch(args, cur_level);
  }
  // Serial.print("3) new_mode: "); Serial.println((int) new_mode); Serial.flush();
  if (new_mode) {
    if (previous && previous != new_mode) {
      delete previous;
    }
    // Serial.print("4) new_mode: "); Serial.println((int) new_mode); Serial.flush();
    return new_mode;
  } else {
    return previous;
  }
}

BlindyRGB *BlindyRGB::new_mode_from_scratch(char * args, unsigned char cur_level) {
  BlindyRGB *new_mode = NULL;
  if (_num_leds > 0){
    switch (args[0]) {
    case BlindyRgbSolid::code:
      new_mode = new BlindyRgbSolid();
      // Serial.print("2.1) new_mode: "); Serial.println((int) new_mode); Serial.flush();
      break;
    case BlindySparkle::code:
      new_mode = new BlindySparkle();
      // Serial.print("2.1) new_mode: "); Serial.println((int) new_mode); Serial.flush();
      break;
    case BlindyCylon::code:
      new_mode = new BlindyCylon();
      // Serial.print("2.2) new_mode: "); Serial.println((int) new_mode); Serial.flush();
      break;
    // default:
      // Serial.print("2.3) Ruh Roh! "); Serial.println((int) new_mode); Serial.flush();
    }
  }
  return new_mode;
}

BlindyRGB *BlindyRGB::next_command(char * args){
    return NULL;
}

int BlindyRGB::rand_lim(int limit){
// return a random number between 0 and limit inclusive.
  int divisor = RAND_MAX/(limit+1);
  int retval;

  do { 
      retval = rand() / divisor;
  } while (retval >= limit);

  return retval;
}

BlindyRgbSolid::BlindyRgbSolid() {
  _next_action = millis();
}

unsigned char BlindyRgbSolid::new_brightness(){
  clear_values();
  _next_action = millis() + _do_nothing_duration;
  return 0;
}

BlindySparkle::BlindySparkle() {
  _duration = 50;
  _next_action = millis();
}

unsigned char BlindySparkle::new_brightness() {
  clear_values();
  leds[rand_lim(_num_leds)] = pulse[rand_lim(NUM_PULSES)];
  _next_action += _duration;
  return 0;
}
BlindyRGB *BlindySparkle::next_command(char * args){
  if (args[0] == BlindySparkle::code)
    return this;
  else
    return NULL;
}

BlindyCylon::BlindyCylon() {
  _mid_sweep_duration = 20;
  _between_sweep_duration = _mid_sweep_duration * 15;
  _swipe_edge = EYE_SIZE;
  _swipe_len = _num_leds + _swipe_edge + _swipe_edge;
  _next_action = millis();
}

void BlindyCylon::assign(int pos, uint16_t color){
  if (pos >= 0 && pos < _num_leds)
    leds[pos] = color;
}

unsigned char BlindyCylon::new_brightness() {
  clear_values();
  for (int i=0; i<EYE_SIZE; i++) {
    if (_forward)
      assign(_position - _swipe_edge - i, cylon_eye[i]);
    else
      assign(_position - _swipe_edge + i, cylon_eye[i]);
  }
  int _wait = _mid_sweep_duration;
  if (_forward) {
    _position++;
    if (_position>=_swipe_len) { // I'm at the end going forward
      _position = _swipe_len;
      _forward = false;
    }
  }else{
    _position--;
    if (_position<0) { // I'm at the end coming back
      _position = 0;
      _forward = true;
      _wait = _between_sweep_duration;
    }
  }
  _next_action += _wait;
  return 0;
}
BlindyRGB *BlindyCylon::next_command(char * args){
  if (args[0] == BlindyCylon::code)
    return this;
  else
    return NULL;
}
