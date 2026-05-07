#include <Arduino.h>
#include "Servos.h" // Asegúrate de que este es el nombre correcto de tu header[cite: 1, 2]

void setup() {
  Serial.begin(115200);
  initPWM(); 
  analogReadResolution(12);
  
}

void loop() {
  int lectura32 = analogRead(32);
  int lectura33 = analogRead(33);
  // Conversión a voltaje (asumiendo 3.3V de referencia)
  float v32 = (lectura32 * 3.3) / 4095.0;
  float v33 = (lectura33 * 3.3) / 4095.0;

  // Salida por terminal
  Serial.print("GPIO 32: "); Serial.print(lectura32);
  Serial.print(" ("); Serial.print(v32); Serial.print("V)");
  
  Serial.print(" | ");
  
  Serial.print("GPIO 33: "); Serial.print(lectura33);
  Serial.print(" ("); Serial.print(v33); Serial.println("V)");
  
    wRuedas(20,20,20);
    delay(10); // Pequeña pausa para no saturar la CPU
}