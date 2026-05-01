//MAC Emisor B0:CB:D8:D7:2F:28
//MAC Receptor F4:65:0B:E7:EB:D0 

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// El LED está en el pin 13
const int LED_PIN = 13;

typedef struct struct_message {
    bool encender;
} struct_message;

struct_message incomingData;

// Función que se ejecuta al recibir datos
void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
    memcpy(&incomingData, data, sizeof(incomingData));
    
    if (incomingData.encender) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED ENCENDIDO");
    } else {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED APAGADO");
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Empezar apagado

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Receptor listo. Esperando comandos...");
}

void loop() {
    // Nada aquí, todo ocurre en el callback
}