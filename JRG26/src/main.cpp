#include <Arduino.h>
#include <Wire.h>
#include "Laser.h"
#include "Servos.h"
#include <PS4Controller.h>
#include "esp_system.h"

//---- VAR. GLOBALES ----//
int16_t laser = 0;

int16_t dist_activacion = 800;
unsigned long t0;
unsigned long Tb = 2500;
const unsigned long T = 10;   // periodo en ms
unsigned long t_inicio = 0;

typedef enum {
  REPOSO,
  E1,
  E2,
  E3,
  E4,
  E5
} Estado_t;

Estado_t estado_actual = REPOSO;


//---- PROTOTIPOS DE FUNCIONES ----//
void actualizar_sensores();
void cambiarEstado(Estado_t nuevo_estado);
bool comprobar_laser(int16_t dist);

int16_t leer_laser();

void ruedasDcha(int velocidad);

void reposo();
void buscar_d();
void buscar_d_despacio();
void buscar_i();
void buscar_i_despacio();
void atacar();

void asignar_estados();

void setup() {
  Serial.begin(115200);
  delay(300);

  // --- PS4 ---
  PS4.begin();
  Serial.println("PS4.begin() hecho.");

  Serial.println("\n--- Test VL53L1X (1 sensor) ---");
  if (!inicializar_LASER()) {
    Serial.println("ERROR: inicializar_LASER() fallo. Revisa cableado/XSHUT.");
    while (1) delay(1000);
  }

  initPWM();
}

void loop() {
  t_inicio = millis();
  actualizar_sensores();

  bool detectado = comprobar_laser(laser);

  switch (estado_actual) {
    case REPOSO:
      reposo();
      if (PS4.isConnected() && PS4.Cross()) cambiarEstado(E1);
      break;

    case E1:
      buscar_i();
      if (detectado) cambiarEstado(E2);
      if (PS4.isConnected() && PS4.Triangle()) cambiarEstado(REPOSO);
      break;
    
    case E2:
      buscar_i();
      if (!detectado){ cambiarEstado(E3); t0 = millis();}
      if (PS4.isConnected() && PS4.Triangle()) cambiarEstado(REPOSO);
      break;

    case E3:
      buscar_d_despacio();
      if (detectado && (millis() - t0 >= 500)) cambiarEstado(E4);
      if (millis() - t0 >= Tb) cambiarEstado(E1);
      if (PS4.isConnected() && PS4.Triangle()) cambiarEstado(REPOSO);
      break;

    case E4:
      atacar();
      if (!detectado) cambiarEstado(E1);
      if (PS4.isConnected() && PS4.Triangle()) cambiarEstado(REPOSO);
      break;

    default:
      cambiarEstado(REPOSO);
      break;
  }

  while (millis() - t_inicio < T) {
    delay(1);
  }
}

void cambiarEstado(Estado_t nuevo_estado) {
  Serial.println(nuevo_estado);
  estado_actual = nuevo_estado;
}

void actualizar_sensores() {
  laser = leer_laser();
}

int16_t leer_laser() {
  int16_t d = leer_LASER(0x29);   // 1 solo sensor, dirección por defecto
  Serial.print("Distancia: ");
  Serial.println(d);
  return d;
}

bool comprobar_laser(int16_t dist) {
  return (dist != -1) && (dist < dist_activacion);
}

void ruedasDcha(int v) {
  ruedaDelDcha(v);
  ruedaTrasDcha(v);
}

void reposo() {
  ruedasIzda(0);
  ruedasDcha(0);
}

//no se usa
void buscar_d() {
  ruedasIzda( 55);
  ruedasDcha(-55);
}

void buscar_d_despacio() {
  ruedasIzda( 45);
  ruedasDcha(-45);
}

void buscar_i() {
  ruedasIzda(-65);
  ruedasDcha( 65);
}

// no se usa
void buscar_i_despacio() {
  ruedasIzda(-45);
  ruedasDcha( 45);
}

void atacar() {
  ruedasIzda( 80);
  ruedasDcha( 80);
}
