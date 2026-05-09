#include <Arduino.h>
#include "Servos.h" // Asegúrate de que este es el nombre correcto de tu header[cite: 1, 2]

void setup() {
  Serial.begin(115200);
  initPWM(10); 
  
}

void loop() {
    wRuedas(20,20,20);
    delay(10); // Pequeña pausa para no saturar la CPU
}