#include <Arduino.h>
#include <Wire.h>
#include "Laser_2.h"
#include "Servos.h"
#include <QTRSensors.h>
#include <Ticker.h>

// ============================================================
// VELOCIDADES DEL ROBOT
// ============================================================

#define B_D_DCHA -25
#define B_D_IZDA  25

#define B_D_DCHA_DESPACIO -15
#define B_D_IZDA_DESPACIO  15

#define B_I_DCHA  25
#define B_I_IZDA -25

#define B_I_DCHA_DESPACIO  15
#define B_I_IZDA_DESPACIO -15

#define A_DCHA 20
#define A_IZDA 20

#define R_DCHA -20
#define R_IZDA -20

// ============================================================
// PINES
// ============================================================

#define PIN_LED 2

// ============================================================
// SENSORES QTR / SIGUELÍNEAS
// ============================================================

#define UMBRAL_ON 3500
#define NUM_SENSORES 2
#define MIN_SENSORES_LINEA 2

const uint8_t sensorPins[NUM_SENSORES] = {32, 33};
QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORES];

// ============================================================
// MOTORES
// ============================================================
int CONSIGNA_DCHA = 0;
int CONSIGNA_IZDA = 0;

// ============================================================
// TIEMPOS
// ============================================================
uint32_t t0 = 0;
uint32_t t_atacado   = 0;
uint32_t T_ATAQUE    = 1288;
uint32_t T_RETROCESO = 1288;
uint32_t T_REPOSO    = 800;

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
  INICIO,
  REPOSO,
  E1,
  E2,
  E3,
  E4,
  E5,
  E6,
  E7,
  E8
} Estado_t;

volatile Estado_t estado_actual = INICIO;

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
void print_sensores_linea();

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


/*
  qtr.setTypeAnalog();
  qtr.setSensorPins(sensorPins, NUM_SENSORES);

  delay(500);



  for (uint16_t i = 0; i < 200; i++)
  {
    qtr.calibrate();
  }

    Serial.println("Inicializacion completada.");
  */
  ticker_motores.attach_ms(10, aplicar_motores);
  ticker_laser.attach_ms(50, ticker_comprobar_laser);
  //ticker_siguelineas.attach_ms(15, ticker_ve_linea);
  ticker_mef.attach_ms(100, aplicar_MEF);
  
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
/*
  print_sensores_linea();
*/
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
    case INICIO:
      // PARA EL CENTRO DE CULO UNOS 100 cm
      // PASAMOS A REPOSO CUANDO LO COMPLETEMOS
      // PARA 100cm 322 ms a 20 de velocidad
      if (t0 == 0) {
        t0 = millis();
      }
      CONSIGNA_DCHA = -20;
      CONSIGNA_IZDA = -20;
      if (millis() - t0 >= 600){ 
        estado_actual = REPOSO;
        t0 = millis();
      }
    break;

    case REPOSO:
      if (millis() - t0 >= T_REPOSO) { 
         estado_actual = E1;
      }
      estado_actual = E1;
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;
      break;

    case E1:
      // BUSQUEDA HASTA VER
      if (laser_detectado) {
        estado_actual = E2;
        t0 = millis();
      }
      CONSIGNA_DCHA = B_I_DCHA;
      CONSIGNA_IZDA = B_I_IZDA;
      break;

    case E2:
      // BUSQUEDA HASTA NO VER
      if (!laser_detectado) {
        estado_actual = E3;
      }
      CONSIGNA_DCHA = B_I_DCHA;
      CONSIGNA_IZDA = B_I_IZDA;
    break;
    
    case E3:
      // BUSQUEDA HASTA VOLVER A VER
      if (laser_detectado) {
        estado_actual = E4;
        t0 = millis();
      }
      CONSIGNA_DCHA = B_D_DCHA_DESPACIO;
      CONSIGNA_IZDA = B_D_IZDA_DESPACIO;
    break;

    case E4:
      //hace un avance de 322 * 4 = 1288 
      if (millis() - t0 >= T_ATAQUE) {
        estado_actual = E5;
        t0 = millis();
      }
      CONSIGNA_DCHA = A_DCHA;
      CONSIGNA_IZDA = A_IZDA;
      break;

    case E5:
      // Retroceder el mismo tiempo que ha estado atacando
      if (millis() - t0 >= T_RETROCESO) {
        estado_actual = REPOSO;
        t_atacado = 0;
      }
      CONSIGNA_DCHA = R_DCHA;
      CONSIGNA_IZDA = R_IZDA;
      break;

    default:
      estado_actual = REPOSO;
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;
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
  //digitalWrite(PIN_LED, linea_detectada ? HIGH : LOW);
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

void print_sensores_linea() {
  static uint32_t t_print = 0;

  if (millis() - t_print >= 200) {
    t_print = millis();

    // Lectura usada por QTR
    qtr.read(sensorValues);

    Serial.print("QTR[0] GPIO32: ");
    Serial.print(sensorValues[0]);

    Serial.print(" | QTR[1] GPIO33: ");
    Serial.print(sensorValues[1]);

    Serial.print(" | linea_detectada: ");
    Serial.print(linea_detectada);

    Serial.print(" | umbral: ");
    Serial.println(UMBRAL_ON);
  }
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
    case INICIO: return "INICIO";
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