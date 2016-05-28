#ifndef BlindyRGB_h
#define BlindyRGB_h

#include "Blindy.h"
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

class BlindyRGB: public Blindy
{
public:
  static void initRGB(int num_leds, int rgb_pin);
  static BlindyRGB *new_command(char * args, BlindyRGB *previous);
  static BlindyRGB *new_mode_from_scratch(char * args, unsigned char cur_level);
  
  BlindyRGB * next_command(char * args);
  
  static uint16_t * leds; // TODO get rid of this
  static Adafruit_NeoPixel * strip;
  static void write_rgbs(); //TODO get rid of this
  static void clear_values(); //TODO get rid of this

protected:
  int rand_lim(int limit);
  static int _num_leds;
};

class BlindyRgbSolid: public BlindyRGB {
public:
  static const char code = 'D';
  BlindyRgbSolid();
  unsigned char new_brightness();
};

class BlindySparkle: public BlindyRGB {
public:
  static const char code = 'A';
  BlindySparkle();
  unsigned char new_brightness();
  BlindyRGB *next_command(char * args);
private:
  int _duration = 20;
  int _off_duration;
};

class BlindyCylon: public BlindyRGB {
public:
  static const char code = 'B';
  BlindyCylon();
  unsigned char new_brightness();
  BlindyRGB *next_command(char * args);
private:
  int _mid_sweep_duration;
  int _between_sweep_duration;
  int _swipe_edge = 2; // This gets overwritten by EYE_SIZE
  int _swipe_len;
  int _position = 0; // This is the position on the whole swipe (larger then num_leds)
  bool _forward = true;
  void assign(int pos, uint16_t color);
};
#endif
