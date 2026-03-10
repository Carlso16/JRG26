#include <Arduino.h>
#define pinModeMux 19
#define pinMotorChulo1 27
#define pinMotorChulo2 14

void setup() {
  // Inicializa la comunicación serie a 115200 baudios
  Serial.begin(115200);
  pinMode(pinModeMux,OUTPUT);
  pinMode(pinMotorChulo1,OUTPUT);
  pinMode(pinMotorChulo2,OUTPUT);
  digitalWrite(pinMotorChulo1,LOW);
  digitalWrite(pinMotorChulo2,LOW);
  digitalWrite(pinModeMux,LOW);
}

void loop() {
  // Lee los valores brutos de los pines analógicos
  int rawD1 = analogRead(36);  // D1: pin 26
  int rawD2 = analogRead(39);  // D2: pin 25
  int rawD3 = analogRead(34);  // D3: pin 33
  int rawD4 = analogRead(35);  // D4: pin 32
  int rawD5 = analogRead(32);  // D5: pin 35
  int rawD6 = analogRead(33);  // D6: pin 34
  int rawD7 = analogRead(25);   // D7: pin 39
  int rawD8 = analogRead(26);   // D8: pin 36
  
  // Convierte el valor bruto a voltaje (0 a 3.3V) usando la resolución de 12 bits (4095)
  float voltageD1 = rawD1 * (3.3 / 4095.0);
  float voltageD2 = rawD2 * (3.3 / 4095.0);
  float voltageD3 = rawD3 * (3.3 / 4095.0);
  float voltageD4 = rawD4 * (3.3 / 4095.0);
  float voltageD5 = rawD5 * (3.3 / 4095.0);
  float voltageD6 = rawD6 * (3.3 / 4095.0);
  float voltageD7 = rawD7 * (3.3 / 4095.0);
  float voltageD8 = rawD8 * (3.3 / 4095.0);
  
  // Imprime los valores de tensión en una sola línea
  Serial.print("D1: ");
  Serial.print(voltageD1, 2);
  Serial.print(" V\tD2: ");
  Serial.print(voltageD2, 2);
  Serial.print(" V\tD3: ");
  Serial.print(voltageD3, 2);
  Serial.print(" V\tD4: ");
  Serial.print(voltageD4, 2);
  Serial.print(" V\tD5: ");
  Serial.print(voltageD5, 2);
  Serial.print(" V\tD6: ");
  Serial.print(voltageD6, 2);
  Serial.print(" V\tD7: ");
  Serial.print(voltageD7, 2);
  Serial.print(" V\tD8: ");
  Serial.println(voltageD8, 2);
  
  // Espera 1 segundo antes de la siguiente lectura
  delay(1000);
}