#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

struct W {
    float dd;
    float di;
    float td;
    float ti;
};

struct PulseCount {
    float dd;
    float di;
    float td;
    float ti;
    float ddAnt;
    float diAnt;
    float tdAnt;
    float tiAnt;
};

void initEncoders();
W readW();
PulseCount readCount();

#endif

//ejemplo de uso
/*
#include <Arduino.h>
#include <Encoder.h>

void setup() {
    initEncoders();
    Serial.begin(115200);

}

void loop() {
    W vel = readW();
    Serial.print("Wdd: ");
    Serial.println(vel.dd);
    delay(100);
}

*/