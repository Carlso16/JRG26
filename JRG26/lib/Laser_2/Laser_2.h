#pragma once
#include <Arduino.h>

// =================== PINES ESP ===================
#ifndef LASER_I2C_SDA
#define LASER_I2C_SDA 21
#endif

#ifndef LASER_I2C_SCL
#define LASER_I2C_SCL 22
#endif

#ifndef LASER_XSHUT
#define LASER_XSHUT 15
#endif

// Si no usas IRQ, se deja en -1
#ifndef LASER_IRQ
#define LASER_IRQ -1
#endif

#ifndef LASER_ADDR
#define LASER_ADDR 0x29
#endif

// =================== FUNCIONES ===================

bool inicializar_LASER();

int16_t leer_LASER(uint8_t direccion_i2c);

bool comprobar_LASER(uint8_t direccion_i2c, int16_t umbral_mm);