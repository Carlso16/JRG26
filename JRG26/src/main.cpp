#include <Arduino.h>
#include "Servos.h" // Asegúrate de que este es el nombre correcto de tu header[cite: 1, 2]

void setup() {
  Serial.begin(115200);
  initPWM(); 
  
}

void loop() {
    wRuedas(50,50,50);
    delay(10); // Pequeña pausa para no saturar la CPU
}