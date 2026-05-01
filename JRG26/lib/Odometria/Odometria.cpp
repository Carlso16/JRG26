#include "Odometria.h"
#include "Encoder.h"
#include <math.h>

#define WHEEL_R_CM  3.385f
#define TRACK_L_CM  21.5f
#define PPR         340.0f

static float pose_x  = 0.0f;
static float pose_y  = 0.0f;
static float pose_th = 0.0f;

static OdoSample odoLog[LOG_SIZE];
static uint16_t  logIdx  = 0;
static bool      logFull = false;

static uint32_t lastUs = 0;

void initOdometry() {
    pose_x  = 0.0f;
    pose_y  = 0.0f;
    pose_th = 0.0f;
    logIdx  = 0;
    logFull = false;
    lastUs  = micros();
}

// ── updateOdometry() ────────────────────────────────────────────────────────
//   delta_pulsos_d = (td - tdAnt) / 2   promedio lado derecho (ruedas traseras)
//   delta_pulsos_i = (ti - tiAnt) / 2   promedio lado izquierdo
//
//   delta_d = 2π · delta_pulsos_d / PPR              [rad]
//   delta_i = 2π · delta_pulsos_i / PPR              [rad]
//
//   s_d = R · delta_d                                [cm]
//   s_i = R · delta_i                                [cm]
//
//   Δs  = (s_d + s_i) / 2
//   Δθ  = (s_d - s_i) / L
//
//   x  += Δs · cos(θ + Δθ/2)
//   y  += Δs · sin(θ + Δθ/2)
//   θ  += Δθ
void updateOdometry() {
    if (micros() - lastUs < 1000) return;
    lastUs += 1000;

    PulseCount p = readCount();

    float dPulse_d = (p.td - p.tdAnt) * 0.5f;
    float dPulse_i = (p.ti - p.tiAnt) * 0.5f;

    float delta_d = (2.0f * M_PI * dPulse_d) / PPR;
    float delta_i = (2.0f * M_PI * dPulse_i) / PPR;

    float s_d = WHEEL_R_CM * delta_d;
    float s_i = WHEEL_R_CM * delta_i;

    float delta_s  = (s_d + s_i) * 0.5f;
    float delta_th = (s_d - s_i) / TRACK_L_CM;

    pose_x  += delta_s * cosf(pose_th + delta_th * 0.5f);
    pose_y  += delta_s * sinf(pose_th + delta_th * 0.5f);
    pose_th += delta_th;

    if (pose_th >  M_PI) pose_th -= 2.0f * M_PI;
    if (pose_th <= -M_PI) pose_th += 2.0f * M_PI;
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

bool isLogFull() {
    return logFull;
}

void printLog() {
    Serial.println("t_ms,x,y,th");
    for (uint16_t i = 0; i < logIdx; i++) {
        Serial.printf("%lu,%.4f,%.4f,%.4f\n",
                      odoLog[i].t_ms, odoLog[i].x, odoLog[i].y, odoLog[i].th);
    }
}
