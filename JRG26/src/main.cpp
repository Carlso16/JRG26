#include <Arduino.h>
#include "Servos.h" 
#include <Ticker.h>

void parar(int ms);
void linea(float cm);
void girar(int grados);
void cuadrado(float lado);
void rectangulo(float ancho, float alto);
void triangulo(float lado);
void calibrarLinea(int ms);
void calibrarAngulo(int ms);

Ticker tickerControl;

int figura = 0; // 3=cuadrado, 1=rectangulo, 2=triangulo, 0=parado

int spTD = 0;
int spI  = 0;

float msPorCm  = 31.7f; //   2000ms / 63cm = 31.74ms por cm
float msPorGrado = 6.67f; // 600ms / 90° = 6.67ms por grado
int msCalibCm = 2000; 
int msCalibGrado = 2000; 


void IRAM_ATTR controlLoop() { wRuedas(0, spTD, spI); }
void setup() {
  Serial.begin(115200);
  initPWM(); 
  tickerControl.attach_ms(10, controlLoop);
}

void loop() {
  if      (figura == 0) parar(1000);
  else if (figura == 1) rectangulo(30, 10);
  else if (figura == 2) triangulo(25);
  else if (figura == 3) cuadrado(40);
  else if (figura == 99) calibrarLinea(msCalibCm);  
  else if (figura == 98) calibrarAngulo(msCalibGrado); 
  parar(10000);
}


// ----------------------------------------------------------------------------------------------------------FUNCIONES BÁSICAS-----------------------------------------------------------------------------------------------------------------------------
void parar(int ms) {
  spTD = 0; spI = 0;
  delay(ms);
}

void linea(float cm) {
  spTD = 20; spI = 20;
  delay((int)(cm * msPorCm));
  parar(200);
}

void girar(int grados) {
  spTD = 20; spI = -20;
  delay((int)(grados * msPorGrado));
  parar(200);
}
// ------------------------------------------------------------------------------------------------------------FIGURAS---------------------------------------------------------------------------------------------------------------------------

void cuadrado(float lado) {
  for(int i = 0; i < 4; i++) {
    linea(lado);
    girar(90);
  }
}

void rectangulo(float ancho, float alto) {
  for(int i = 0; i < 2; i++) {
    linea(ancho);
    girar(90);
    linea(alto);
    girar(90);
  }
}

void triangulo(float lado) {
  for(int i = 0; i < 3; i++) {
    linea(lado);
    girar(120);
  }
}

// ------------------------------------------------------------------------------------------------------------CALIBRACIÓN---------------------------------------------------------------------------------------------------------------------------

void calibrarLinea(int ms) {
  spTD = 20; spI = 20;
  delay(ms);
  parar(20000);
}

void calibrarAngulo(int ms) {
  spTD = 20; spI = -20;
  delay(ms);
  parar(20000);
}
