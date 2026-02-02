#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

struct W {
    float dd;
    float di;
    float td;
    float ti;
};

void initEncoders();
W readW();

#endif

