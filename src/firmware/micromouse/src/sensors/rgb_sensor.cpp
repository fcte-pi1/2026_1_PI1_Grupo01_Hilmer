#include "rgb_sensor.h"

// Define TCS3200 pins
#define S0_PIN 15
#define S1_PIN 2
#define S2_PIN 0
#define S3_PIN 4
#define OUT_PIN 16

void initRGBSensor() {
#ifdef ARDUINO
    pinMode(S0_PIN, OUTPUT);
    pinMode(S1_PIN, OUTPUT);
    pinMode(S2_PIN, OUTPUT);
    pinMode(S3_PIN, OUTPUT);
    pinMode(OUT_PIN, INPUT);

    // Setting frequency scaling to 20%
    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);
#endif
}

RGBColor readRGB() {
    RGBColor color = {0, 0, 0};

#ifdef ARDUINO
    // Read Red
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, LOW);
    color.r = pulseIn(OUT_PIN, LOW);

    // Read Green
    digitalWrite(S2_PIN, HIGH);
    digitalWrite(S3_PIN, HIGH);
    color.g = pulseIn(OUT_PIN, LOW);

    // Read Blue
    digitalWrite(S2_PIN, LOW);
    digitalWrite(S3_PIN, HIGH);
    color.b = pulseIn(OUT_PIN, LOW);
#endif

    return color;
}
