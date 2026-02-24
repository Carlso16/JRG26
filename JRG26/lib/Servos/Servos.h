#ifndef MOTORES_H
#define MOTORES_H

#include <Arduino.h>

// --- Definición de Pines ---
#define PIN_PWM1 17      
#define PIN_PWM2 16  
#define PIN_PWM3 27 // Delantera derecha
#define PIN_PWM4 14      
#define PIN_PWM7 13 // Trasera derecha
#define PIN_PWM8 12

// --- Parámetros PWM ---
#define FRECUENCIA 5000  // Hz
#define RESOLUCION 8     // 0-255

// --- Canales PWM ---
#define CANAL_1 0
#define CANAL_2 1
#define CANAL_3 2
#define CANAL_4 3
#define CANAL_7 4
#define CANAL_8 5

// --- Prototipos de Funciones ---
void initPWM();
void ruedaDelDcha(int velocidad);
void ruedasIzda(int velocidad);
void ruedaTrasDcha(int velocidad);

#endif