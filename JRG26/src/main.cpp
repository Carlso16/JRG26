#define _USE_MATH_DEFINES
#include <Arduino.h>
#include "Encoder.h"
#include "Odometria.h"
// ── Parámetros del robot ────────────────────────────────────────────────────
#define WHEEL_R_CM  3.385f   // radio de rueda [cm]
#define TRACK_L_CM  21.5f   // distancia entre ruedas [cm]
#define PPR         340.0f  // pulsos por vuelta (340 PPR x2 full-quad)


// ── Pose (variables globales) ───────────────────────────────────────────────
static float pose_x  = 0.0f;  // [cm]
static float pose_y  = 0.0f;  // [cm]
static float pose_th = 0.0f;  // [rad]

void readOdometry(float *x, float *y, float *th);
void updateOdometry();
void logOdometry();

// ── Log de odometría ─────────────────────────────────────────────────────────
#define LOG_SIZE 2000

static OdoSample odoLog[LOG_SIZE];
static uint16_t  logIdx = 0;
static bool      logFull = false;

static uint32_t lastUs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  initEncoders();

  Serial.print("Test");
}

void loop() {
  updateOdometry();
  logOdometry();

  // Cuando el log esté lleno, volcarlo por Serial
  if (logFull) {
    Serial.println("=== LOG ODOMETRIA ===");
    Serial.println("t_ms,x,y,th");
    for (int i = 0; i < LOG_SIZE; i++) {
      Serial.printf("%lu,%.4f,%.4f,%.4f\n",
        odoLog[i].t_ms,
        odoLog[i].x,
        odoLog[i].y,
        odoLog[i].th);
    }
    Serial.println("=== FIN LOG ===");
    logFull = false;  // resetea para volver a loggear
    logIdx  = 0;
  }
}


void updateOdometry() {
    if (micros() - lastUs >= 1000) { lastUs += 1000;  //100 ms
      PulseCount p = readCount();
      //Serial.printf("pulsos trasera derecha: d=%.2f\n", p.td);
      //Serial.printf("pulso traser izq: i=%.2f\n",p.ti);
      //Serial.printf("pulsos delantera derecha: d=%.2f\n", p.dd);
      //Serial.printf("pulso delantera izq: i=%.2f\n",p.di);
      // solo con ruedas traseras
      float dPulse_d = (p.td - p.tdAnt) * 0.5f;
      float dPulse_i = (p.ti - p.tiAnt) * 0.5f;
      // Serial.printf("pulsos: ti=%.2f, tiANT=%.2f\n", p.ti, p.tiAnt);

      float delta_d = (2.0f * M_PI * dPulse_d) / PPR;
      float delta_i = (2.0f * M_PI * dPulse_i) / PPR;

      float s_d = WHEEL_R_CM * delta_d;
      float s_i = WHEEL_R_CM * delta_i;

      float delta_s  = (s_d + s_i) * 0.5f;
      float delta_th = (s_d - s_i) / TRACK_L_CM;

      pose_x  += delta_s * cosf(pose_th + delta_th * 0.5f);
      pose_y  += delta_s * sinf(pose_th + delta_th * 0.5f);
      pose_th += delta_th;

      // Normalizar θ a (-π, π]
      if (pose_th >  M_PI) pose_th -= 2.0f * M_PI;
      if (pose_th <= -M_PI) pose_th += 2.0f * M_PI;
    }
}

void readOdometry(float *x, float *y, float *th) {
    *x  = pose_x;
    *y  = pose_y;
    *th = pose_th;
}

void logOdometry() {
    if (logFull) return;
    odoLog[logIdx++] = { millis(), pose_x, pose_y, pose_th };
    if (logIdx >= LOG_SIZE) logFull = true;
}
