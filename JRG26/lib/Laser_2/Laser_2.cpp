#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L1X.h"
#include "Laser_2.h"

// =================== OBJETO ======================

static Adafruit_VL53L1X g_vl53 = Adafruit_VL53L1X(LASER_XSHUT, LASER_IRQ);

// =================== VARIABLES GLOBALES ==========

static bool g_iniciado = false;
static int16_t g_ultima_medida = -1;

// =================== FUNCIONES ====================

bool inicializar_LASER() {
  g_iniciado = false;
  g_ultima_medida = -1;

  Wire.begin(LASER_I2C_SDA, LASER_I2C_SCL);
  Wire.setClock(400000);
  delay(20);

  pinMode(LASER_XSHUT, OUTPUT);

  digitalWrite(LASER_XSHUT, LOW);
  delay(10);

  digitalWrite(LASER_XSHUT, HIGH);
  delay(20);

  if (!g_vl53.begin(LASER_ADDR, &Wire)) {
    Serial.print("Fallo init VL53L1X en 0x");
    Serial.println(LASER_ADDR, HEX);
    Serial.print("VL status: ");
    Serial.println(g_vl53.vl_status);
    return false;
  }

  if (!g_vl53.startRanging()) {
    Serial.println("No se pudo iniciar el ranging del VL53L1X");
    Serial.print("VL status: ");
    Serial.println(g_vl53.vl_status);
    return false;
  }

  g_vl53.setTimingBudget(50);

  g_vl53.VL53L1X_SetDistanceMode(2);

  g_iniciado = true;

  Serial.println("VL53L1X inicializado OK");
  return true;
}

int16_t leer_LASER(uint8_t direccion_i2c) {
  if (!g_iniciado) {
    return -1;
  }

  if (direccion_i2c != LASER_ADDR) {
    return -1;
  }

  if (!g_vl53.dataReady()) {
    return g_ultima_medida;
  }

  int16_t distancia = g_vl53.distance();

  g_vl53.clearInterrupt();

  if (distancia == -1) {
    g_ultima_medida = -1;
    return -1;
  }

  g_ultima_medida = distancia;
  return distancia;
}

bool comprobar_LASER(uint8_t direccion_i2c, int16_t umbral_mm) {
  int16_t distancia = leer_LASER(direccion_i2c);

  if (distancia < 0) {
    return false;
  }

  return distancia <= umbral_mm;
}