#include <Arduino.h>
#include <Wire.h>
#include "Laser_2.h"
#include "Servos.h"
#include <QTRSensors.h>
#include <Ticker.h>

// ============================================================
// VELOCIDADES DEL ROBOT
// ============================================================

#define B_D_DCHA -15
#define B_D_IZDA  15

#define B_D_DCHA_DESPACIO -10
#define B_D_IZDA_DESPACIO  10

#define B_I_DCHA  15
#define B_I_IZDA -15

#define B_I_DCHA_DESPACIO  10
#define B_I_IZDA_DESPACIO -10

#define A_DCHA 30
#define A_IZDA 30

#define R_DCHA -30
#define R_IZDA -30

// ============================================================
// PINES
// ============================================================

#define PIN_LED 2

// ============================================================
// SENSORES QTR / SIGUELÍNEAS
// ============================================================

#define UMBRAL_ON 3000
#define NUM_SENSORES 1
#define MIN_SENSORES_LINEA 1

const uint8_t sensorPins[NUM_SENSORES] = {4};

QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORES];

// ============================================================
// MOTORES
// ============================================================
int CONSIGNA_DCHA = 0;
int CONSIGNA_IZDA = 0;



// ============================================================
// LÁSER
// ============================================================

#define DIR_LASER 0x29

volatile int16_t laser = -1;
int16_t dist_activacion = 800;
volatile bool laser_detectado = false;

// ============================================================
// LÍNEA
// ============================================================

volatile bool linea_detectada = false;

// ============================================================
// TICKERS
// ============================================================

Ticker ticker_laser;
Ticker ticker_siguelineas;
Ticker ticker_motores;
Ticker ticker_mef;

// ============================================================
// ESTADOS
// ============================================================

typedef enum {
  REPOSO,
  E1,
  E2,
  E3,
  E4,
  E5
} Estado_t;

volatile Estado_t estado_actual = REPOSO;

// ============================================================
// PROTOTIPOS
// ============================================================

void initSUMO();

int16_t leer_laser();
bool veLinea();

void mover_robot(float velocidadDcha, float velocidadIzda);
void aplicar_MEF();
void aplicar_motores();

const char* estadoToTexto(Estado_t estado);

void ticker_comprobar_laser();
void ticker_ve_linea();

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n--- Inicializando robot ---");

  if (!inicializar_LASER()) {
    Serial.println("ERROR: inicializar_LASER() fallo. Revisa cableado/XSHUT/I2C.");
    while (1) {
      delay(1000);
    }
  }

  initPWM(10);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  qtr.setTypeAnalog();
  qtr.setSensorPins(sensorPins, NUM_SENSORES);

  delay(500);

  Serial.println("Inicializacion completada.");

  ticker_motores.attach_ms(10, aplicar_motores);
  ticker_laser.attach_ms(50, ticker_comprobar_laser);
  ticker_siguelineas.attach_ms(15, ticker_ve_linea);
  ticker_mef.attach_ms(100, aplicar_MEF);
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  /*
  Serial.print("Laser: ");
  Serial.print(laser);

  Serial.print(" | Laser detectado: ");
  Serial.print(laser_detectado);

  Serial.print(" | Linea detectada: ");
  Serial.print(linea_detectada);
  
  Serial.print(" | Estado actual: ");
  Serial.println(estadoToTexto(estado_actual));

  delay(100);
  */
}

// ============================================================
// MEF
// ============================================================

void aplicar_MEF() {
  switch (estado_actual) {
    case REPOSO:
      if (!laser_detectado) {
        estado_actual = E1;
      }
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;
      break;

    case E1:
      if (laser_detectado) {
        estado_actual = E2;
      }
      CONSIGNA_DCHA = B_I_DCHA;
      CONSIGNA_IZDA = B_I_IZDA;
      break;

    case E2:
      if (!laser_detectado) {
        estado_actual = E1;
      }
      CONSIGNA_DCHA = A_DCHA;
      CONSIGNA_IZDA = A_IZDA;
      break;

    default:
      estado_actual = REPOSO;
      break;
  }
}

// ============================================================
// SENSORES
// ============================================================

void ticker_comprobar_laser() {
  // Usa directamente la función de la librería Laser_2
  laser_detectado = comprobar_LASER(DIR_LASER, dist_activacion);

  // Guarda también la distancia actual para poder imprimirla si quieres
  laser = leer_LASER(DIR_LASER);

  digitalWrite(PIN_LED, laser_detectado ? HIGH : LOW);
}

int16_t leer_laser() {
  return leer_LASER(DIR_LASER);
}

void ticker_ve_linea() {
  linea_detectada = veLinea();
}

bool veLinea() {
  qtr.read(sensorValues);

  uint8_t sensoresActivos = 0;

  for (uint8_t i = 0; i < NUM_SENSORES; i++) {
    if (sensorValues[i] > UMBRAL_ON) {
      sensoresActivos++;

      if (sensoresActivos >= MIN_SENSORES_LINEA) {
        return true;
      }
    }
  }

  return false;
}

// ============================================================
// MOTORES
// ============================================================

void aplicar_motores() {
  mover_robot(CONSIGNA_DCHA, CONSIGNA_IZDA);
}

void mover_robot(float velocidadDcha, float velocidadIzda) {
  wRuedas(velocidadDcha, velocidadDcha, velocidadIzda);
}

// ============================================================
// TEXTO ESTADOS
// ============================================================

const char* estadoToTexto(Estado_t estado) {
  switch (estado) {
    case REPOSO: return "REPOSO";
    case E1:     return "E1";
    case E2:     return "E2";
    case E3:     return "E3";
    case E4:     return "E4";
    case E5:     return "E5";
    default:     return "DESCONOCIDO";
  }
}

// ============================================================
// INIT
// ============================================================

void initSUMO() {

}