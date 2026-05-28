#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xEC, 0xE3, 0x34, 0x99, 0xEA, 0x7C};

// Estructura para enviar el estado de ambos botones
typedef struct struct_message {
    bool boton1; // Pin 23
    bool boton2; // Pin 26
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

const int PIN_1 = 32;
const int PIN_2 = 26;

// Variables para antirebote
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 30;
bool lastState1 = HIGH;
bool lastState2 = HIGH;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_1, INPUT_PULLUP);
    pinMode(PIN_2, INPUT_PULLUP);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) return;

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void loop() {
    bool currentState1 = digitalRead(PIN_1);
    bool currentState2 = digitalRead(PIN_2);

    // Si algún botón cambia de estado
    if (currentState1 != lastState1 || currentState2 != lastState2) {
        
        // Filtro antirebote simple
        if ((millis() - lastDebounceTime) > debounceDelay) {
            
            // Lógica inversa por INPUT_PULLUP: LOW = Pulsado (true)
            myData.boton1 = (currentState1 == LOW);
            myData.boton2 = (currentState2 == LOW);

            esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
            
            lastState1 = currentState1;
            lastState2 = currentState2;
            lastDebounceTime = millis();

            Serial.printf("Boton 23: %d | Boton 26: %d\n", myData.boton1, myData.boton2);
        }
    }
}