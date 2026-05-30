#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Definición de pines de salida
const int GRUPO_A_PIN1 = 25;
const int GRUPO_A_PIN2 = 16;
const int GRUPO_B_PIN1 = 4;
const int GRUPO_B_PIN2 = 26;

typedef struct struct_message {
    bool boton1;
    bool boton2;
} struct_message;

struct_message incomingData;

void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
    memcpy(&incomingData, data, sizeof(incomingData));

    // Control Grupo A (Mientras pin 23 esté pulsado)
    Serial.println("");
    Serial.print("Boton1 : ");
    Serial.println(incomingData.boton1);
    digitalWrite(GRUPO_A_PIN1, incomingData.boton1 ? LOW : HIGH);
    digitalWrite(GRUPO_A_PIN2, incomingData.boton1 ? LOW : HIGH);

    // Control Grupo B (Mientras pin 26 esté pulsado)
    
    Serial.print("Boton2 : ");
    Serial.println(incomingData.boton2);
    digitalWrite(GRUPO_B_PIN1, incomingData.boton2 ? LOW : HIGH);
    digitalWrite(GRUPO_B_PIN2, incomingData.boton2 ? LOW : HIGH);
}

void setup() {
    Serial.begin(115200);

    pinMode(GRUPO_A_PIN1, OUTPUT);
    pinMode(GRUPO_A_PIN2, OUTPUT);
    pinMode(GRUPO_B_PIN1, OUTPUT);
    pinMode(GRUPO_B_PIN2, OUTPUT);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) return;

    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Receptor de 4 canales listo...");
}

void loop() {
    // El procesamiento es manejado por la interrupción de ESP-NOW
}