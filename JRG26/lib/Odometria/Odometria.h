#pragma once
#include <Arduino.h>

#define LOG_SIZE 2000

struct OdoSample {
    uint32_t t_ms;
    float x;   // [cm]
    float y;   // [cm]
    float th;  // [rad]
};

void initOdometry();
void updateOdometry();
void readOdometry(float *x, float *y, float *th);
void logOdometry();
bool isLogFull();
void printLog();
