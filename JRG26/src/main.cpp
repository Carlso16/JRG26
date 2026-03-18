#include <Arduino.h>
#include <Servos.h>

#define pinModeMux 19
#define unmbralON 2800

#include <QTRSensors.h>
const uint8_t sensorPins[] = {26,25,33,32,35,34,39,36};
uint16_t  sensorValues[8];
boolean perdidaDeLinea = false;
boolean perdidaDeLineaAnt = false;
boolean estadoSensores[8];
boolean estadoSensoresAnt[8];
boolean* ptr = estadoSensores;    
boolean* ptrAnt = estadoSensoresAnt;
QTRSensors qtr;

int setPoint = 3500;
int vBase = 65; //orden 20
int accion = 0;
int error = 0;
int errorAnt = 0;
int errorCong = 0;
float Kp = 0.05; //orden 0.05
float volantazo = 110/Kp;
float Kd = 0.35;
float Ki = 0;
int sentidoi = 0;
int sentidod = 0;

void aplicarAccion(int accion);
void leerSensores();

void setup() {
  // Inicializa la comunicación serie a 115200 baudios
  Serial.begin(115200);
  pinMode(pinModeMux,OUTPUT);
  pinMode(2,OUTPUT);
  initPWM();
  digitalWrite(pinModeMux,LOW);
  qtr.setTypeAnalog();
  qtr.setSensorPins(sensorPins,8);
  delay(500);
  ruedaDelDcha(0);
    for (uint16_t i = 0; i < 200; i++)
  {
    qtr.calibrate();
  }
  ruedaDelDcha(0);
  delay(500);
}

void loop() {
  uint16_t position =  qtr.readLineBlack(sensorValues);
  qtr.read(sensorValues);
  for (uint8_t i = 0; i < 7; i++) {
    if (sensorValues[i] > unmbralON) {
      estadoSensores[i] = true;  // Sensor detecta la línea (ON)
    } else {
      estadoSensores[i] = false; // Sensor no detecta la línea (OFF)
    }
  }
  perdidaDeLinea = true;
  for(int i = 0; i < 7; i++){
    perdidaDeLinea = perdidaDeLinea && not(estadoSensores[i]);
  }
  if(perdidaDeLinea && !perdidaDeLineaAnt){
    if(estadoSensoresAnt[0] || estadoSensoresAnt[1]){
      errorCong = volantazo;
    }
    else{
      errorCong = -volantazo;
    }
  }
  digitalWrite(2,perdidaDeLinea);
  error = setPoint - position;
  if(perdidaDeLinea)
    error = errorCong;
  accion = Kp*error + Kd*(error-errorAnt) + Ki*(error+errorAnt); // + Ki*(error + errorAnt) + Kd*(error - errorAnt) creo
  aplicarAccion(accion);
  //leerValoresK();
  errorAnt = error;
  perdidaDeLineaAnt = perdidaDeLinea;
  for(int i = 0; i < 7; i++){
    estadoSensoresAnt[i] = estadoSensores[i];
  }
  
}
void aplicarAccion(int accion){
  int velIzda = constrain(vBase + accion, -100, 100);
  int velDcha = constrain(vBase - accion, -100, 100);
  ruedasIzda(velIzda);
  ruedaTrasDcha(velDcha); 
  ruedaDelDcha(velDcha);
}


