/**
 * Blindies.h
 * Mike McCracken
 * 
 * A library for driving remotely controlled performance costume lights
 */

#ifndef Blindy_h
#define Blindy_h

#include "Arduino.h"

class Blindy
{
public:
  static const char solo_on_code = '(';
  static const char solo_off_code = ')';
  static const char roll_call_code = '.';
  
  static void seed_random(int seed);
  // new_command is what the driver calls when a new command has come in - this function processes that command
  static Blindy *new_command(char * args, Blindy *previous);
  // new_mode_from_scratch builds a new Blindy object from just the command and the current light level
  static Blindy *new_mode_from_scratch(char * args, unsigned char cur_level);
  
  // new_brightness is called when we've made it to the time to do the next action. This increments the state, returning the next brightness to display (it is called by the driver)
  virtual unsigned char new_brightness();
  // next_command gives the blindy object an opportunity to set up the next blindy object or adjust it's own state and return itself. This is called from new_command
  virtual Blindy *next_command(char * args);

  bool is_time_to_act();
  int cur_level();
  
  static void set_mic_pin(int mic_pin);
  static void addressable_config(int num_leds, int led_pin);

protected:
  int duration_from_speed(unsigned char speed);
  void calculate_increment_values_for_variable_fade(unsigned char start, unsigned char target, unsigned char speed);
  void calculate_increment_values_for_fixed_fade(unsigned char start, unsigned char target, unsigned char speed);
  bool is_at_end_after_increment();
  
  const int _min_increment_duration = 20;
  const int _do_nothing_duration = 5000;
  
  int _cur_level;
  int _brightness;
  unsigned long _next_action;
  int _increment_amount;
  int _increment_duration;
  int _target;

private:
  static int _mic_pin;
  static int _num_leds;
  static bool _solo_mode;
};

class BlindySet: public Blindy {
public:
  static const char code = '0';
  BlindySet(unsigned char level);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindyFadeVariable: public Blindy {
public:
  static const char code = '1';
  BlindyFadeVariable(unsigned char cur_level, unsigned char target, unsigned char speed);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindyFadeFixed: public Blindy {
public:
  static const char code = '2';
  BlindyFadeFixed(unsigned char cur_level, unsigned char target, unsigned char speed);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindyOnePulse: public Blindy {
public:
  static const char code = '7';
  BlindyOnePulse(unsigned char cur_level, unsigned char brightness, unsigned char attack, unsigned char decay);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
private:
  bool _going_up;
  unsigned char _down_speed;
};

class BlindyBlinkSteady: public Blindy {
public:
  static const char code = '3';
  BlindyBlinkSteady(unsigned char brightness, unsigned char speed, unsigned char duty);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
private:
  bool _on;
  unsigned int _duration;
  unsigned int _on_time;
  unsigned int _off_time;
};

class BlindyPulseSteady: public Blindy {
public:
  static const char code = '4';
  BlindyPulseSteady(unsigned char cur_level, unsigned char brightness, unsigned char speed, unsigned char range);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
private:
  bool _going_up;
  unsigned int _bottom;
  unsigned int _speed;
};

class BlindyBlinkRandom: public Blindy {
public:
  static const char code = '5';
  BlindyBlinkRandom(unsigned char brightness, unsigned char speed, unsigned char duty);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindyPulseRandom: public Blindy {
public:
  static const char code = '6';
  BlindyPulseRandom(unsigned char cur_level, unsigned char brightness, unsigned char speed, unsigned char range);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindySoundSensitive: public Blindy {
public:
  static const char code = '8';
  BlindySoundSensitive(unsigned char brightness, unsigned char decay, int analog_pin);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
  const int low_threshold = 5; // lower threshold from which to trigger
  const int high_threshold = 860; // upper threshold from which to trigger
private:
  int _analog_pin;
  bool _hit;
};

class BlindySparkle: public Blindy {
public:
  static const char code = 'A';
  BlindySparkle(unsigned char brightness, unsigned char speed);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

class BlindyCylon: public Blindy {
public:
  static const char code = 'B';
  BlindyCylon(unsigned char brightness, unsigned char speed);
  unsigned char new_brightness();
  Blindy *next_command(char * args);
};

#endif