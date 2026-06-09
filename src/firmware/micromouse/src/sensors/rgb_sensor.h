#ifndef RGB_SENSOR_H
#define RGB_SENSOR_H

#ifdef TARGET_ESP32
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
