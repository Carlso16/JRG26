#pragma once
#include <Arduino.h>

// =================== PINES ESP ===================
#ifndef LASER_I2C_SDA
#define LASER_I2C_SDA 21
#endif

#ifndef LASER_I2C_SCL
#define LASER_I2C_SCL 22
#endif

#ifndef LASER_XSHUT_1
#define LASER_XSHUT_1 18
#endif

#ifndef LASER_XSHUT_2
#define LASER_XSHUT_2 19
#endif

#ifndef LASER_ADDR_1
#define LASER_ADDR_1 0x30
#endif

#ifndef LASER_ADDR_2
#define LASER_ADDR_2 0x31
#endif

// =================== FUNCIONES ===================

bool inicializar_LASER(); 
//devuelve true si se inicializan los dos sensores

int16_t leer_LASER(uint8_t direccion_i2c); 
// Devuelve distancia en mm si OK, o -1 si:
// - no inicializado
// - fuera de rango / error de lectura
// - dirección no válida
