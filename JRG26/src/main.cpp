#include <Arduino.h>
#include <Encoder.h>

void setup() {
    initEncoders();
    Serial.begin(115200);

}

void loop() {
    W vel = readW();
}