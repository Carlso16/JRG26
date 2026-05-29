#include <Arduino.h>
#include <Wire.h>
#include "Servos.h"
#include <PS4Controller.h>
#include "esp_system.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_err.h"
#include "ESP32Servo.h"


//---- VAR. GLOBALES ----//
int RSY = 0;
int LSX = 0;

int v_izquierda = 0;
int v_derecha = 0;


//---- SERVO ----//
#define PIN_SERVO 23

#define CANAL_SERVO 6
#define FRECUENCIA_SERVO 50
#define RESOLUCION_SERVO 16

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2600

#define ANGULO_MIN 0
#define ANGULO_MAX 180

#define PASO_SERVO 9
#define INTERVALO_SERVO_MS 30

int anguloServo = 90;
unsigned long tUltimoServo = 0;


//---- PROTOTIPOS DE FUNCIONES ----//
void ruedasDcha(int velocidad);
void removePairedDevices();
void printDeviceAddress();

void initServo();
void escribirServo(int angulo);
void actualizarServoConMando();


void setup() {
  Serial.begin(115200);
  delay(300);

  PS4.begin();
  Serial.println("PS4.begin() hecho.");

  Serial.print("This device MAC is: ");
  printDeviceAddress();

  removePairedDevices();
  Serial.println("Dispositivos emparejados eliminados.");
  
  initPWM();      // Si tu versión usa initPWM() sin parámetro, cambia esta línea por initPWM();
  initServo();
}


void loop() {
  if (PS4.isConnected()) {
    RSY = PS4.RStickY();
    LSX = PS4.LStickX();

    if (RSY < 10 && RSY > -10) RSY = 0;
    if (LSX < 10 && LSX > -10) LSX = 0;

    int velocidad = map(RSY, -128, 127, -100, 100);
    int giro = map(LSX, -128, 127, -100, 100);

    v_izquierda = velocidad + giro;
    v_derecha   = velocidad - giro;

    if (PS4.R1()) v_izquierda = v_izquierda * 0.4;
    if (PS4.R1()) v_derecha   = v_derecha * 0.4;

    v_izquierda = constrain(v_izquierda, -100, 100);
    v_derecha   = constrain(v_derecha, -100, 100);  

    ruedasDcha(v_derecha);
    ruedasIzda(v_izquierda);

    actualizarServoConMando();

    // Si quieres seguir viendo velocidades, descomenta esto:
    /*
    Serial.printf(
      "Vel. Izda: %d, Vel. Dcha: %d\n",
      v_izquierda,
      v_derecha
    );
    */
  }

  delay(10);
}


void initServo() {
  ledcSetup(CANAL_SERVO, FRECUENCIA_SERVO, RESOLUCION_SERVO);
  ledcAttachPin(PIN_SERVO, CANAL_SERVO);

  escribirServo(anguloServo);

  Serial.print("Servo inicial: ");
  Serial.print(anguloServo);
  Serial.println(" grados");
}


void escribirServo(int angulo) {
  if (angulo < 45) angulo = 45;
  if (angulo > 175) angulo = 175;
  angulo = constrain(angulo, ANGULO_MIN, ANGULO_MAX);

  int pulso_us = map(angulo, 0, 180, SERVO_MIN_US, SERVO_MAX_US);

  uint32_t dutyMax = (1UL << RESOLUCION_SERVO) - 1;
  uint32_t duty = ((uint64_t)pulso_us * dutyMax) / 20000UL;

  ledcWrite(CANAL_SERVO, duty);
}


void actualizarServoConMando() {
  unsigned long ahora = millis();

  if (ahora - tUltimoServo < INTERVALO_SERVO_MS) {
    return;
  }

  int nuevoAngulo = anguloServo;

  // Flecha izquierda: abrir poco a poco
  if (PS4.Left() && !PS4.Right()) {
    nuevoAngulo += PASO_SERVO;
  }

  // Flecha derecha: cerrar poco a poco
  else if (PS4.Right() && !PS4.Left()) {
    nuevoAngulo -= PASO_SERVO;
  }

  nuevoAngulo = constrain(nuevoAngulo, ANGULO_MIN, ANGULO_MAX);

  if (nuevoAngulo != anguloServo) {
    anguloServo = nuevoAngulo;
    escribirServo(anguloServo);

    Serial.print("Servo: ");
    Serial.print(anguloServo);
    Serial.println(" grados");
  }

  tUltimoServo = ahora;
}


void ruedasDcha(int v) {
  ruedaDelDcha(v);
  ruedaTrasDcha(v);
}


void removePairedDevices() {
  uint8_t pairedDeviceBtAddr[20][6];
  int count = esp_bt_gap_get_bond_device_num();
  esp_bt_gap_get_bond_device_list(&count, pairedDeviceBtAddr);

  for (int i = 0; i < count; i++) {
    esp_bt_gap_remove_bond_device(pairedDeviceBtAddr[i]);
  }
}


void printDeviceAddress() {
  const uint8_t* mac = esp_bt_dev_get_address();

  Serial.printf(
    "%02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
  );
}