#include <Arduino.h>
#include <Servos.h>
#include <BluetoothSerial.h>
#include <QTRSensors.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>

#define pinModeMux 19
#define vBaseSat 40
#define setPoint 3500

const uint8_t sensorPins[] = {26,25,33,32,35,34,39,36};
uint16_t  sensorValues[8];
boolean perdidaDeLinea = false;
boolean perdidaDeLineaAnt = false;
boolean estadoSensores[8];
boolean estadoSensoresAnt[8];
boolean* ptr = estadoSensores;    
boolean* ptrAnt = estadoSensoresAnt;
QTRSensors qtr;
BluetoothSerial SerialBT;
String MAC_recibida;

float vBase = 40; //orden 20
int accion = 0;
int error = 0;
int errorAnt = 0;
int errorCong = 0;
float Kp = 0.05; //orden 0.05
float volantazo = 250/Kp;
float Kd = 0.35;
float Ki = 0;
float Kv = 0.05;
float kvi = 1;
float umbralON = 2600;
int vBase0 = 70;
int Iv = 0;

void aplicarAccion(int accion);
void leerSensores();
void recibirDatosBluetooth();

void setup() {
  // Inicializa la comunicación serie a 115200 baudios
  Serial.begin(115200);
  SerialBT.begin("SIGUEPOP_ESP32");
  Serial.println("Bluetooth SPP iniciado");
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
    if (sensorValues[i] > umbralON) {
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
  vBase = vBase0 - Kv*abs(error);
  if(vBase < vBaseSat) vBase = vBaseSat;
  if(perdidaDeLinea)
    error = errorCong;
  accion = Kp*error + Kd*(error-errorAnt) + Ki*(error+errorAnt); 
  if(perdidaDeLinea){
    Iv += error*kvi;
    accion += Iv;
  } 
  else
    Iv = 0;
  aplicarAccion(accion);
  //leerValoresK();
  errorAnt = error;
  perdidaDeLineaAnt = perdidaDeLinea;
  for(int i = 0; i < 7; i++){
    estadoSensoresAnt[i] = estadoSensores[i];
  }
  recibirDatosBluetooth();
  delay(100);
}
void aplicarAccion(int accion){
  int velIzda = constrain((int)vBase + accion, -100, 100);
  int velDcha = constrain((int)vBase - accion, -100, 100);
  ruedasIzda(velIzda);
  ruedaTrasDcha(velDcha); 
  ruedaDelDcha(velDcha);
}
void recibirDatosBluetooth() {
  if (SerialBT.available() <= 0) return;

  String mensaje = SerialBT.readStringUntil('\n');
  mensaje.trim();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, mensaje);

  if (error) {
    SerialBT.print("ERROR JSON: ");
    SerialBT.println(error.c_str());
    return;
  }

  MAC_recibida = doc["MAC"] | "";

  Kd = doc["KD"] | 0.0;
  Kp = doc["KP"] | 0.0;
  Ki = doc["KI"] | 0.0;
  Kv = doc["Kv"] | 0.0;
  kvi = doc["Kvi"] | 0.0;
  vBase = doc["Vbase"] | 0.0;
  volantazo = doc["Volantazo"] | 0.0;
  umbralON = doc["Umbral"] | 0.0;

  Serial.print("DATOS RECIBIDOS OK");
  Serial.print(" | KP="); Serial.print(Kp);
  Serial.print(" | KI="); Serial.print(Ki);
  Serial.print(" | KD="); Serial.print(Kd);
  Serial.print(" | Kv="); Serial.print(Kv);
  Serial.print(" | Kvi="); Serial.print(kvi);
  Serial.print(" | Vbase="); Serial.print(vBase);
  Serial.print(" | Volantazo="); Serial.print(volantazo);
  Serial.print(" | Umbral="); Serial.println(umbralON);

  
  SerialBT.print("DATOS RECIBIDOS OK");
  SerialBT.print(" | KP="); SerialBT.print(Kp);
  SerialBT.print(" | KI="); SerialBT.print(Ki);
  SerialBT.print(" | KD="); SerialBT.print(Kd);
  SerialBT.print(" | Kv="); SerialBT.print(Kv);
  SerialBT.print(" | Kvi="); SerialBT.print(kvi);
  SerialBT.print(" | Vbase="); SerialBT.print(vBase);
  SerialBT.print(" | Volantazo="); SerialBT.print(volantazo);
  SerialBT.print(" | Umbral="); SerialBT.println(umbralON);
}