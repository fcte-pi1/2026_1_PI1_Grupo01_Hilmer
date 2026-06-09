#ifndef RGB_SENSOR_H
#define RGB_SENSOR_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

struct RGBColor {
    int r;
    int g;
    int b;
};

void initRGBSensor();
RGBColor readRGB();

#endif
