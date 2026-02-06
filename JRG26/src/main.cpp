#include <Arduino.h>
#include <Encoder.h>

#define PIN_L_FWD 17
#define PIN_L_BWD 16
#define PIN_TD_FWD 13
#define PIN_TD_BWD 12
#define PIN_DD_FWD 14
#define PIN_DD_BWD 27


void setup() {
    Serial.begin(115200);

    // Configuración de pines de motores
    pinMode(PIN_L_FWD, OUTPUT);
    pinMode(PIN_L_BWD, OUTPUT);
    pinMode(PIN_TD_FWD, OUTPUT);
    pinMode(PIN_TD_BWD, OUTPUT);
    pinMode(PIN_DD_FWD, OUTPUT);
    pinMode(PIN_DD_BWD, OUTPUT);

    // Inicializar librería de encoders externa
    initEncoders();

    // Nota: ESP32 maneja PWM automáticamente con analogWrite en versiones recientes del core.
    // Si usas una versión muy antigua, necesitarías configurar ledcSetup.
    analogWriteResolution(8); // Resolución 0-255
    digitalWrite(PIN_L_FWD,HIGH);
}

void loop() {
   
}

