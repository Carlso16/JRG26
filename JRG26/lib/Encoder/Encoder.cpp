#include <Arduino.h>
#include <ESP32Encoder.h>
#include "Encoder.h"

#define pinA1 26
#define pinB1 25
#define pinA2 32
#define pinB2 33
#define pinA3 34
#define pinB3 39
#define pinA4 35
#define pinB4 36
#define pinMux 19

//340 pulsos por vuelta

long ultimaPosicion[] = {0,0,0,0};
long nuevaPosicion[] = {0,0,0,0};
float deltaTita[] = {0,0,0,0};
float deltaT = 0;

unsigned long Tant = 0;
unsigned long T = 0;


ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;
ESP32Encoder* encoders[4] = { &encoder1, &encoder2, &encoder3, &encoder4 };

void initEncoders() {
    // Mux alto para pasar los encoders
    pinMode(pinMux, OUTPUT);
    digitalWrite(pinMux, HIGH);
    //declaracion libreria
    
    encoder1.attachFullQuad(pinA1,pinB1);
    encoder2.attachFullQuad(pinA2,pinB2);
    encoder3.attachFullQuad(pinA3,pinB3);
    encoder4.attachFullQuad(pinA4,pinB4);
    //inicializacion
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4.setCount(0);

    Serial.println("Encoders inicializados");
}

W readW(){
    W velAng;
    for(int i=0;i<4;i++){
        ultimaPosicion[i] = encoders[i]->getCount();
        Serial.println(ultimaPosicion[i]);
    }
    return velAng;
}

