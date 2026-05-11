#include "Servos.h"
#include "Encoder.h"

#define Kp 0.4
#define Ki 20
#define Vmax 11

//100ms
//Kp 0.15
//Ki 1
//20ms
//Kp 0.4
//Ki 5

void accDD(int V);
void accTD(int V);
void accI (int V);

float i = 0;
float IDD = 0;
float ITD = 0;
float II = 0;
float Ts = 0;
bool satDD = false;
bool satTD = false;
bool satI = false;

void initPWM(float ms) {
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

  initEncoders();

  //tiempo de ejecución
  Ts = ms/1000;
}

void wRuedas(int wDD,int wTD,int wI){
  //Lectura w
  W wRuedas = readW();
  //Cambio de rad/s a 0-100
  wRuedas.dd *= 2.083; 
  wRuedas.td *= -2.083;
  wRuedas.ti *= -2.083;

  //Calculo error
  float eDD = wDD - wRuedas.dd;
  float eTD = wTD - wRuedas.td;
  float eTI = wI - wRuedas.ti;
  //Calculo de acción integral con sntiwindup (solo si no está saturado)
  if(!satDD)
    IDD += Ki * eDD * Ts;
  if(!satTD)
    ITD += Ki * eTD * Ts;
  if(!satI)
    II += Ki * eTI * Ts;
  //Calculo de accion
  float VDD = Kp*eDD + IDD;
  float VTD = Kp*eTD + ITD;
  float VI = Kp*eTI + II;
  //Saturación de acción
  //Delantera Derecha
  if(VDD > Vmax){
    satDD = true;
    VDD = Vmax;
  }
  else if (VDD < -Vmax){
    satDD = true; 
    VDD = -Vmax; 
  }
  else 
    satDD = false;
  //Trasera derecha
  if(VTD > Vmax){
    satTD = true;
    VTD = Vmax;
  }
  else if (VTD < -Vmax){
    satTD = true; 
    VTD = -Vmax; 
  }
  else 
    satTD = false;
  //Izquierdas
  if(VI > Vmax){
    satI = true;
    VI = Vmax;
  }
  else if (VI < -Vmax){
    satI = true; 
    VI = -Vmax; 
  }
  else 
    satI = false;

    i++;
  if(i > 50){
    i = 0;
    // --- IMPRESIÓN SIMPLE ---
    Serial.print("DD -> SP:"); Serial.print(wDD); 
    Serial.print(" w:"); Serial.print(wRuedas.dd); 
    Serial.print(" Err:"); Serial.print(eDD); 
    Serial.print(" V:"); Serial.println(VDD);

    Serial.print("TD -> SP:"); Serial.print(wTD); 
    Serial.print(" w:"); Serial.print(wRuedas.td); 
    Serial.print(" Err:"); Serial.print(eTD); 
    Serial.print(" V:"); Serial.println(VTD);

    Serial.print("IZ -> SP:"); Serial.print(wI); 
    Serial.print(" w:"); Serial.print(wRuedas.ti); 
    Serial.print(" Err:"); Serial.print(eTI); 
    Serial.print(" V:"); Serial.println(VI);

    Serial.print("Var -> Ts:"); Serial.println(Ts); 

    
    Serial.println("---"); // Separador para cada ciclo
  }
  //Aplicar acción
  accDD(VDD);
  accTD(VTD);
  accI(VI);
}
/*i++;
  if(i > 50){
    i = 0;
    // --- IMPRESIÓN SIMPLE ---
    Serial.print("DD -> SP:"); Serial.print(wDD); 
    Serial.print(" w:"); Serial.print(wRuedas.dd); 
    Serial.print(" Err:"); Serial.print(eDD); 
    Serial.print(" V:"); Serial.println(VDD);

    Serial.print("TD -> SP:"); Serial.print(wTD); 
    Serial.print(" w:"); Serial.print(wRuedas.td); 
    Serial.print(" Err:"); Serial.print(eTD); 
    Serial.print(" V:"); Serial.println(VTD);

    Serial.print("IZ -> SP:"); Serial.print(wI); 
    Serial.print(" w:"); Serial.print(wRuedas.di); 
    Serial.print(" Err:"); Serial.print(eTI); 
    Serial.print(" V:"); Serial.println(VI);
    
    Serial.println("---"); // Separador para cada ciclo
  }*/
void accDD(int V) {
    int duty = 255 * abs(V) / Vmax;
    duty = constrain(duty, 0, 255);
    if (V > 0) {
      ledcWrite(CANAL_8, duty);
      ledcWrite(CANAL_7, 0);
    } else {
      ledcWrite(CANAL_8, 0);
      ledcWrite(CANAL_7, duty);
    }  
}

void accI(int V) {
    int duty = 255 * abs(V) / Vmax;
    duty = constrain(duty, 0, 255);
    if (V > 0) {
      ledcWrite(CANAL_2, duty);
      ledcWrite(CANAL_1, 0);
    } else {
      ledcWrite(CANAL_2, 0);
      ledcWrite(CANAL_1, duty);
    }  
}
    


void accTD(int V) {
    int duty = 255 * abs(V) / Vmax;
    duty = constrain(duty, 0, 255);
    if (V > 0) {
      ledcWrite(CANAL_3, duty);
      ledcWrite(CANAL_4, 0);
    } else {
      ledcWrite(CANAL_3, 0);
      ledcWrite(CANAL_4, duty);
    }  
}














/*
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
    */