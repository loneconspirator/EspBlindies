/**
 * Blindies.cpp
 * Mike McCracken
 * 
 * A library for driving remotely controlled performance costume lights
 */

#include "Blindy.h"
#include "Arduino.h"

void Blindy::seed_random(int seed) {
  srand(seed);
}

int Blindy::_mic_pin = -1;
int Blindy::_num_leds = 0;
bool Blindy::_solo_mode = false;

void Blindy::set_mic_pin(int pin) {
  _mic_pin = pin;
}

void Blindy::addressable_config(int num_leds, int led_pin) {
  // Do stuff here...
}

Blindy *Blindy::new_command(char * args, Blindy *previous) {
  Blindy *new_mode = NULL;
  unsigned char cur_level = 0;
  if (previous) {
    new_mode = previous->next_command(args);
    cur_level = previous->cur_level();
  }
  if (!new_mode) new_mode = new_mode_from_scratch(args, cur_level);
  if (new_mode) {
    if (previous && previous != new_mode) {
      delete previous;
    }
    return new_mode;
  } else {
    return previous;
  }
}

Blindy *Blindy::new_mode_from_scratch(char * args, unsigned char cur_level) {
  Blindy *newMode = NULL;
  switch (args[0]) {
    case BlindySet::code:
    newMode = new BlindySet((unsigned char)args[1]);
    break;
    case BlindyFadeVariable::code:
    newMode = new BlindyFadeVariable(cur_level, (unsigned char)args[1], (unsigned char)args[2]);
    break;
    case BlindyFadeFixed::code:
    newMode = new BlindyFadeFixed(cur_level, (unsigned char)args[1], (unsigned char)args[2]);
    break;
    case BlindyOnePulse::code:
    newMode = new BlindyOnePulse(cur_level, (unsigned char)args[1], (unsigned char)args[2], (unsigned char)args[3]);
    break;
    case BlindyBlinkSteady::code:
    newMode = new BlindyBlinkSteady((unsigned char)args[1], (unsigned char)args[2], (unsigned char)args[3]);
    break;
    case BlindyPulseSteady::code:
    newMode = new BlindyPulseSteady(cur_level, (unsigned char)args[1], (unsigned char)args[2], (unsigned char)args[3]);
    break;
    case BlindyBlinkRandom::code:
    newMode = new BlindyBlinkRandom((unsigned char)args[1], (unsigned char)args[2], (unsigned char)args[3]);
    break;
    case BlindyPulseRandom::code:
    newMode = new BlindyPulseRandom(cur_level, (unsigned char)args[1], (unsigned char)args[2], (unsigned char)args[3]);
    break;
    case BlindySoundSensitive::code:
    if (_mic_pin >= 0)
      newMode = new BlindySoundSensitive((unsigned char)args[1], (unsigned char)args[2], _mic_pin);
    break;
    case solo_on_code:
    _solo_mode = true;
    break;
    case solo_off_code:
    _solo_mode = false;
    break;
    case roll_call_code:
    // can I do this here?
    break;
    case BlindySparkle::code:
    if (_num_leds > 0)
      newMode = new BlindySparkle((unsigned char)args[1], (unsigned char)args[2]);
    break;
    case BlindyCylon::code:
    if (_num_leds > 0)
      newMode = new BlindyCylon((unsigned char)args[1], (unsigned char)args[2]);
    break;
  }
  return newMode;
}

bool Blindy::is_time_to_act() {
  return (_next_action <= millis());
}
int Blindy::cur_level() {
  return _cur_level;
}
int Blindy::duration_from_speed(unsigned char speed){
  return (270 - speed) * (270 - speed) / 8;
}


void Blindy::calculate_increment_values_for_variable_fade(unsigned char start, unsigned char target, unsigned char speed) {
  int total_millis = duration_from_speed(speed);
  _cur_level = start;
  _target = (int) target;
  int diff = (int) target - _cur_level;
  bool neg = false;
  _next_action = millis();
  if (diff == 0) {
    _increment_amount = 0;
    _increment_duration = _do_nothing_duration;
    return;
  } else {
    if (diff < 0){
      neg = true;
      diff *= -1;
    }
    _increment_duration = (total_millis / diff);
    if (_increment_duration < _min_increment_duration) {
      _increment_duration = _min_increment_duration;
      _increment_amount = (_min_increment_duration * diff) / total_millis;
      if (_increment_amount == 0) _increment_amount = 1;
    }else{
      _increment_amount = 1;
    }
    _increment_amount *= (neg ? -1 : 1);
  }
}
void Blindy::calculate_increment_values_for_fixed_fade(unsigned char start, unsigned char target, unsigned char speed) {
  _cur_level = start;
  _target = (int) target;
  if (_cur_level == _target) {
    _increment_amount = 0;
    _increment_duration = _do_nothing_duration;
    return;
  } else {
    _increment_amount = (int) speed / 2 - 60;
    if (_increment_amount < 1) _increment_amount = 1;
    _increment_duration = ((int) speed / -3) + 45;
    if (_increment_duration < _min_increment_duration)
      _increment_duration = _min_increment_duration;
    if (_target < _cur_level) _increment_amount *= -1;
  }
}

bool Blindy::is_at_end_after_increment() {
  bool done = false;
  _next_action += _increment_duration;
  _cur_level += _increment_amount;
  if (_increment_amount < 0 ? _cur_level <= _target : _cur_level >= _target) {
    done = true;
    _cur_level = _target;
  }
  return done;
}

int Blindy::random_vary(int value, int amount){
  int newVal = value + (rand() % amount) - (amount / 2);
  if (newVal < 0) newVal = 0;
  if (newVal > 255) newVal = 255;
  return newVal;
}

BlindySet::BlindySet(unsigned char level){
  _cur_level = (int) level;
  _next_action = millis();
}

unsigned char BlindySet::new_brightness() {
  _next_action += _do_nothing_duration;
  return (unsigned char) _cur_level;
}
Blindy *BlindySet::next_command(char * args) {
  return NULL;
}

BlindyFadeVariable::BlindyFadeVariable(unsigned char cur_level, unsigned char target, unsigned char speed) {
  calculate_increment_values_for_variable_fade(cur_level, target, speed);
  _next_action = millis();
}

unsigned char BlindyFadeVariable::new_brightness() {
  if (is_at_end_after_increment()) {
    _increment_amount = 0;
    _increment_duration = _do_nothing_duration;
  }
  return (unsigned char) _cur_level;
}
Blindy *BlindyFadeVariable::next_command(char * args){
  return NULL;
}

BlindyFadeFixed::BlindyFadeFixed(unsigned char cur_level, unsigned char target, unsigned char speed) {
  calculate_increment_values_for_fixed_fade(cur_level, target, speed);
  _next_action = millis();
}
unsigned char BlindyFadeFixed::new_brightness() {
  if (is_at_end_after_increment()) {
    _increment_amount = 0;
    _increment_duration = _do_nothing_duration;
  }
  return (unsigned char) _cur_level;
}
Blindy *BlindyFadeFixed::next_command(char * args){
  return NULL;
}

BlindyOnePulse::BlindyOnePulse(unsigned char cur_level, unsigned char brightness, unsigned char attack, unsigned char decay) {
  _going_up = true;
  _down_speed = decay;
  calculate_increment_values_for_variable_fade(cur_level, brightness, attack);
  _next_action = millis();
}
unsigned char BlindyOnePulse::new_brightness() {
  if (is_at_end_after_increment()) {
    if (_going_up) {
      calculate_increment_values_for_fixed_fade(_cur_level, 0, _down_speed);
      _going_up = false;
    } else {
      _increment_amount = 0;
      _increment_duration = _do_nothing_duration;
    }
  }
  return (unsigned char) _cur_level;
}
Blindy *BlindyOnePulse::next_command(char * args){
  return NULL;
}

BlindyBlinkSteady::BlindyBlinkSteady(unsigned char brightness, unsigned char speed, unsigned char duty) {
  _brightness = brightness;
  _duration = duration_from_speed(speed);
  _on_time = (_duration * duty) / 255;
  _off_time = _duration - _on_time;
  _on = false;
  _cur_level = 0;
  _next_action = millis();
}

unsigned char BlindyBlinkSteady::new_brightness() {
  if (!_on) {
    _cur_level = _brightness;
    _next_action = _next_action + _on_time;
  } else {
    _cur_level = 0;
    _next_action = _next_action + _off_time;
  }
  _on = !_on;
  return (unsigned char) _cur_level;
}
Blindy *BlindyBlinkSteady::next_command(char * args){
  if (args[0] == BlindyBlinkSteady::code){
    _brightness = (unsigned char)args[1];
    _duration = duration_from_speed((unsigned char)args[2]);
    _on_time = (_duration * (unsigned char)args[3]) / 255;
    _off_time = _duration - _on_time;
    return this;
  } else
    return NULL;
}

BlindyPulseSteady::BlindyPulseSteady(unsigned char cur_level, unsigned char brightness, unsigned char speed, unsigned char range) {
  _brightness = brightness;
  _speed = speed;
  _bottom = (_brightness * (255 - range)) / 255;
  calculate_increment_values_for_variable_fade(cur_level, _brightness, _speed);
  _going_up = true;
  _next_action = millis();
//  Serial.print("Go between ");  Serial.print(_brightness);  Serial.print(" and ");  Serial.println(_bottom);
}

unsigned char BlindyPulseSteady::new_brightness() {
  if (is_at_end_after_increment()) {
    if (_going_up) {
      // I just went up, now I go down
      calculate_increment_values_for_variable_fade(_cur_level, _bottom, _speed);
    } else {
      calculate_increment_values_for_variable_fade(_cur_level, _brightness, _speed);
    }
    _going_up = !_going_up;
  }
  return (unsigned char) _cur_level;
}
Blindy *BlindyPulseSteady::next_command(char * args){
  if (args[0] == BlindyPulseSteady::code) {
    _brightness = (unsigned char)args[1];
    _speed = (unsigned char)args[2];
    _bottom = (_brightness * (255 - (unsigned char)args[3])) / 255;
    return this;
  } else
    return NULL;
}

BlindyBlinkRandom::BlindyBlinkRandom(unsigned char brightness, unsigned char speed, unsigned char duty) {
  _brightness = brightness;
  _speed = speed;
  _duty = duty;
  _first = true;
  _on = false;
  _cur_level = 0;
  _next_action = millis();
}

unsigned char BlindyBlinkRandom::new_brightness() {
  if (_first){
    _cur_level = 0;
    _first = false;
    _next_action = rand() % duration_from_speed(_speed);
  }else if (!_on) {
    unsigned int duration = duration_from_speed(random_vary(_speed, random_variance));
    unsigned int on_time = (duration * _duty) / 255;
    _off_time = duration - on_time;
    _cur_level = _brightness;
    _next_action = _next_action + on_time;
  } else {
    _cur_level = 0;
    _next_action = _next_action + _off_time;
  }
  _on = !_on;
  return (unsigned char) _cur_level;
}
Blindy *BlindyBlinkRandom::next_command(char * args){
  if (args[0] == BlindyBlinkRandom::code){
    _brightness = (unsigned char)args[1];
    _speed = (unsigned char)args[2];
    _duty = (unsigned char)args[3];
    return this;
  } else
    return NULL;
}

BlindyPulseRandom::BlindyPulseRandom(unsigned char cur_level, unsigned char brightness, unsigned char speed, unsigned char range) {
  _brightness = brightness;
  _speed = speed;
  _bottom = (_brightness * (255 - range)) / 255;
  calculate_increment_values_for_variable_fade(cur_level, _brightness, random_vary(_speed, random_variance));
  _going_up = true;
  _next_action = millis();
}

unsigned char BlindyPulseRandom::new_brightness() {
  if (is_at_end_after_increment()) {
    if (_going_up) {
      // I just went up, now I go down
      calculate_increment_values_for_variable_fade(_cur_level, _bottom, random_vary(_speed, random_variance));
    } else {
      calculate_increment_values_for_variable_fade(_cur_level, _brightness, random_vary(_speed, random_variance));
    }
    _going_up = !_going_up;
  }
  return (unsigned char) _cur_level;
}
Blindy *BlindyPulseRandom::next_command(char * args){
  if (args[0] == BlindyPulseRandom::code) {
    _brightness = (unsigned char)args[1];
    _speed = (unsigned char)args[2];
    _bottom = (_brightness * (255 - (unsigned char)args[3])) / 255;
    return this;
  } else
    return NULL;
}

BlindySoundSensitive::BlindySoundSensitive(unsigned char brightness, unsigned char decay, int analog_pin) {
  _brightness = brightness;
  _analog_pin = analog_pin;
  _cur_level = 0;
  _hit = false;
  _next_action = millis();
}

unsigned char BlindySoundSensitive::new_brightness() {
  int val = analogRead(_analog_pin);
  bool hit = (val < low_threshold || val > high_threshold);
//  Serial.print("Sound mode got value: "); Serial.print(val); if (hit) Serial.print(" HIT"); Serial.println();
  _next_action = millis();
  return (unsigned char) (hit ? _brightness : 0);
}
Blindy *BlindySoundSensitive::next_command(char * args){
  return NULL;
}

BlindySparkle::BlindySparkle(unsigned char brightness, unsigned char speed) {
  
}

unsigned char BlindySparkle::new_brightness() {
  return (unsigned char) _cur_level;
}
Blindy *BlindySparkle::next_command(char * args){
  return NULL;
}

BlindyCylon::BlindyCylon(unsigned char brightness, unsigned char speed) {
  
}

unsigned char BlindyCylon::new_brightness() {
  return (unsigned char) _cur_level;
}
Blindy *BlindyCylon::next_command(char * args){
  return NULL;
}

