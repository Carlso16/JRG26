#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// MAC del Receptor
uint8_t broadcastAddress[] = {0xF4, 0x65, 0x0B, 0xE7, 0xEB, 0xD0};

typedef struct struct_message {
    bool encender; 
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Configuración de Pines
const int PIN_ON = 32;
const int PIN_OFF = 26;

// Variables para el filtro antirebote
unsigned long lastDebounceTimeON = 0;  
unsigned long lastDebounceTimeOFF = 0;  
const unsigned long debounceDelay = 50; // Tiempo de estabilidad (ms)

int lastButtonStateON = HIGH;
int lastButtonStateOFF = HIGH;
int stableButtonStateON = HIGH;
int stableButtonStateOFF = HIGH;

void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_ON, INPUT_PULLUP);
    pinMode(PIN_OFF, INPUT_PULLUP);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error al añadir receptor");
        return;
    }
}

void loop() {
    // --- LÓGICA FILTRO PARA PIN_ON (32) ---
    int readingON = digitalRead(PIN_ON);

    if (readingON != lastButtonStateON) {
        lastDebounceTimeON = millis();
    }

    if ((millis() - lastDebounceTimeON) > debounceDelay) {
        if (readingON != stableButtonStateON) {
            stableButtonStateON = readingON;
            // Solo enviamos cuando el estado estable pasa a LOW (pulsado)
            if (stableButtonStateON == LOW) {
                myData.encender = true;
                esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
                Serial.println(">> Comando: ENCENDER");
            }
        }
    }
    lastButtonStateON = readingON;

    // --- LÓGICA FILTRO PARA PIN_OFF (26) ---
    int readingOFF = digitalRead(PIN_OFF);

    if (readingOFF != lastButtonStateOFF) {
        lastDebounceTimeOFF = millis();
    }

    if ((millis() - lastDebounceTimeOFF) > debounceDelay) {
        if (readingOFF != stableButtonStateOFF) {
            stableButtonStateOFF = readingOFF;
            // Solo enviamos cuando el estado estable pasa a LOW (pulsado)
            if (stableButtonStateOFF == LOW) {
                myData.encender = false;
                esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
                Serial.println(">> Comando: APAGAR");
            }
        }
    }
    lastButtonStateOFF = readingOFF;
}