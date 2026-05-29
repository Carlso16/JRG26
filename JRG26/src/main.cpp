#include <Arduino.h>
#include <Wire.h>
#include "Servos.h"
#include <PS4Controller.h>
#include "esp_system.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_err.h"

#include <ESP32Servo.h>


//---- VAR. GLOBALES ----//
int RSY = 0;
int LSX = 0;

int v_izquierda = 0;
int v_derecha = 0;

Servo servo1;

int minUs = 500;
int maxUs = 2600;
int servo1Pin = 23;
int pos = 0; 

//---- PROTOTIPOS DE FUNCIONES ----//
void ruedasDcha(int velocidad);
void removePairedDevices(); // This helps to solve connection issues
void printDeviceAddress();
void init_servo_golf();
void mover_servo(int angle);

void setup() {
  Serial.begin(115200);
  delay(300);

  PS4.begin();
  Serial.println("PS4.begin() hecho.");

  Serial.print("This device MAC is: ");
  printDeviceAddress();

  removePairedDevices();
  Serial.println("Dispositivos emparejados eliminados.");
  
  initPWM();

  init_servo_golf();

}


void loop() {
  if (PS4.isConnected()) {
    RSY = PS4.RStickY();
    LSX = PS4.LStickX();

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

    

    if (PS4.L1()) pos += 1;
    if (PS4.R1()) pos -= 1;

    pos = pos + round(map(PS4.L2Value(), 0, 255, 0, 4));
    pos = pos - round(map(PS4.R2Value(), 0, 255, 0, 4));  
  
    if(pos >= 180) pos = 180;
    if(pos <= 0) pos = 0;
    servo1.write(pos);


    //printf("Vel. Izda: %d, Vel. Dcha: %d, Servo: %d\n ", v_izquierda, v_derecha, pos);

  }

  delay(10);
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



void init_servo_golf(){
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	Serial.begin(115200);
	servo1.setPeriodHertz(50);    
	servo1.attach(servo1Pin, minUs, maxUs);

}