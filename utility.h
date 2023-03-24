#ifndef UTILITY_H
#define UTILITY_H

#include <cstdint>
#include <vector>
#include <math.h>

struct RGB {
  uint8_t red, green, blue;

  RGB() { } // leaving contents uninitialized
  RGB(uint8_t* data):
      blue(*data), green(*(data+1)), red(*(data+2)) {}
  RGB(uint8_t red, uint8_t green, uint8_t blue):
    red(red), green(green), blue(blue)
  { }
  void toPtr(uint8_t* data) {
      *data = blue; *(data+1) = green; *(data+2) = red;
  }
};

extern void RGB2HSV(RGB rgb, int &_H, int &_S, int &_V);
extern double calcVariance(std::vector<double> &v);
extern double calcColorDistance(const RGB &rgb1, const RGB &rgb2);

#endif