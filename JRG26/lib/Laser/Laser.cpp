#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "LASER.h"

// =================== OBJETOS ======================
static Adafruit_VL53L0X g_lox1;
static Adafruit_VL53L0X g_lox2;

// ===================  VARIABLES GLOBALES ==========
static bool g_iniciado = false;


// =================== FUNCIONES ====================
static int16_t leerMM(Adafruit_VL53L0X &lox) {
  VL53L0X_RangingMeasurementData_t m;
  lox.rangingTest(&m, false);

  if (m.RangeStatus == 0 && m.RangeMilliMeter > 0) {
    return (int16_t)m.RangeMilliMeter;
  }
  return -1;
}


bool inicializar_LASER() {
  g_iniciado = false;

  // Inicia I2C
  Wire.begin(LASER_I2C_SDA, LASER_I2C_SCL, 400000);
  delay(20);

  // Control XSHUT
  pinMode(LASER_XSHUT_1, OUTPUT);
  pinMode(LASER_XSHUT_2, OUTPUT);

  // Apaga ambos para evitar conflicto en 0x29
  digitalWrite(LASER_XSHUT_1, LOW);
  digitalWrite(LASER_XSHUT_2, LOW);
  delay(10);

  // ---------- SENSOR 1 ----------
  digitalWrite(LASER_XSHUT_1, HIGH);
  delay(10);

  if (!g_lox1.begin(0x29, false, &Wire)) {
    Serial.println("Fallo init VL53L0X #1 en 0x29");
    return false;
  }
  g_lox1.setAddress(LASER_ADDR_1);
  delay(5);

  // ---------- SENSOR 2 ----------
  digitalWrite(LASER_XSHUT_2, HIGH);
  delay(10);

  if (!g_lox2.begin(0x29, false, &Wire)) {
    Serial.println("Fallo init VL53L0X #2 en 0x29");
    return false;
  }
  g_lox2.setAddress(LASER_ADDR_2);
  delay(5);

  g_iniciado = true;
  Serial.println("VL53L0X: 2 sensores inicializados OK.");
  return true;
}

int16_t leer_LASER(uint8_t direccion_i2c) {
  if (!g_iniciado) return -1;

  if (direccion_i2c == (uint8_t)LASER_ADDR_1) return leerMM(g_lox1);
  if (direccion_i2c == (uint8_t)LASER_ADDR_2) return leerMM(g_lox2);

  return -1;
}


