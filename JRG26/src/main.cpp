#include <Arduino.h>
#include "Servos.h"
#include <Ticker.h>

Ticker servo;

void setup() {
  Serial.begin(115200);
  initPWM(10); 
  servo.attach_ms(10, []() {
    wRuedas(20, 20, 20);
  });
}

void loop() {
    
}