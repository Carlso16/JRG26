#include <Arduino.h>
#include <ESP32Encoder.h>
#include "Encoder.h"

#define pinA1 26
#define pinB1 25
#define pinA2 3//32
#define pinB2 2//33
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
static uint32_t lastUs = 0;


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
        nuevaPosicion[i] = encoders[i]->getCount();
    }
    T = millis();
    deltaT = T - Tant;
    for(int i=0;i<4;i++){
        deltaTita[i] = (2*3.1415f*(nuevaPosicion[i] - ultimaPosicion[i]))/680;
    }
    velAng.di = 1000*deltaTita[1]/deltaT;
    velAng.td = 1000*deltaTita[2]/deltaT;
    velAng.dd = 1000*deltaTita[0]/deltaT;
    velAng.ti = 1000*deltaTita[3]/deltaT;
    Tant = T;
    for(int i=0;i<4;i++){
        ultimaPosicion[i] = nuevaPosicion[i];
    }
    return velAng;
}

PulseCount readCount(){
    static PulseCount pulsos = {0,0,0,0,0,0,0,0}; 

    // Guardar anterior ANTES de leer nuevo
    pulsos.ddAnt = pulsos.dd;
    pulsos.diAnt = pulsos.di;
    pulsos.tdAnt = pulsos.td;
    pulsos.tiAnt = pulsos.ti;
    // micros() - lastUs >= 1000; lastUs += 1000;  //1 ms
    pulsos.di = encoders[1]->getCount();  // era dd → real: di
    pulsos.td = -encoders[2]->getCount(); // era di → real: td (invertido)
    pulsos.ti = -encoders[3]->getCount();  // era td → real: ti
    pulsos.dd = encoders[0]->getCount();  // era ti → real: dd

    return pulsos;
}