#include "Servos.h"

void initPWM() {
  ledcSetup(CANAL_1, FRECUENCIA, RESOLUCION);
  ledcSetup(CANAL_2, FRECUENCIA, RESOLUCION);
  ledcSetup(CANAL_3, FRECUENCIA, RESOLUCION);
  ledcSetup(CANAL_4, FRECUENCIA, RESOLUCION);
  ledcSetup(CANAL_7, FRECUENCIA, RESOLUCION);
  ledcSetup(CANAL_8, FRECUENCIA, RESOLUCION);

  ledcAttachPin(PIN_PWM1, CANAL_1);
  ledcAttachPin(PIN_PWM2, CANAL_2);
  ledcAttachPin(PIN_PWM3, CANAL_3);
  ledcAttachPin(PIN_PWM4, CANAL_4);
  ledcAttachPin(PIN_PWM7, CANAL_7);
  ledcAttachPin(PIN_PWM8, CANAL_8);

  ledcWrite(CANAL_1, 0);
  ledcWrite(CANAL_2, 0);
  ledcWrite(CANAL_3, 0);
  ledcWrite(CANAL_4, 0);
  ledcWrite(CANAL_7, 0);
  ledcWrite(CANAL_8, 0);
}

void ruedaDelDcha(int velocidad) {
    int duty = 255 * abs(velocidad) / 100;
    duty = constrain(duty, 0, 255);
    if (velocidad > 0) {
      ledcWrite(CANAL_8, duty);
      ledcWrite(CANAL_7, 0);
    } else {
      ledcWrite(CANAL_8, 0);
      ledcWrite(CANAL_7, duty);
    }  
}

void ruedasIzda(int velocidad) {
    int duty = 255 * abs(velocidad) / 100;
    duty = constrain(duty, 0, 255);
    if (velocidad > 0) {
      ledcWrite(CANAL_2, duty);
      ledcWrite(CANAL_1, 0);
    } else {
      ledcWrite(CANAL_2, 0);
      ledcWrite(CANAL_1, duty);
    }  
}

void ruedaTrasDcha(int velocidad) {
    int duty = 255 * abs(velocidad) / 100;
    duty = constrain(duty, 0, 255);
    if (velocidad > 0) {
      ledcWrite(CANAL_3, duty);
      ledcWrite(CANAL_4, 0);
    } else {
      ledcWrite(CANAL_3, 0);
      ledcWrite(CANAL_4, duty);
    }  
}