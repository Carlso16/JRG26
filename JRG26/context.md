# Contexto del Firmware — Robot ASTI 2026 (JRG26) v1.0

## Plataforma
- **MCU:** ESP32 (board `esp32dev`)
- **Toolchain:** PlatformIO + Arduino framework
- **Monitor:** 115200 baud
- **Dependencias externas:**
  - `madhephaestus/ESP32Encoder @ ^0.10.1`
  - `adafruit/Adafruit_VL53L0X @ ^1.2.4`

---

## Arquitectura del firmware

```
src/
  main.cpp              — Entry point: setup/loop, prueba de odometría (estado actual)
lib/
  Encoder/              — Lectura de 4 encoders cuadráticos via ESP32Encoder
  Odometria/            — Odometría diferencial + log de muestras
  Servos/               — Control PWM de motores (ledc ESP32)
  Laser/                — Sensores ToF VL53L0X x2 por I2C
  LineFollower/         — STUB (vacío)
  PC/                   — STUB (vacío — comunicación con PC)
  PS4/                  — STUB (vacío — mando PS4)
```

---

## Módulos implementados

### Encoder (`lib/Encoder/`)
Maneja 4 encoders cuadráticos full-quad (340 PPR por vuelta).

| Variable | Encoder | Pines A/B | Nota |
|----------|---------|-----------|------|
| `dd` (delantera derecha) | encoder1 (idx 0) | 26 / 25 | |
| `di` (delantera izquierda) | encoder2 (idx 1) | 32 / 33 | |
| `td` (trasera derecha) | encoder3 (idx 2) | 34 / 39 | **invertido** (`-count`) |
| `ti` (trasera izquierda) | encoder4 (idx 3) | 35 / 36 | **invertido** (`-count`) |

- Pin MUX: GPIO 19 (HIGH para habilitar encoders)
- `readW()` → velocidad angular [rad/s] de cada rueda
- `readCount()` → pulsos acumulados + pulsos anteriores (para delta)

**Nota de cableado importante:** el mapeo físico encoder→rueda está invertido respecto al nombre de la variable (`pulsos.di = encoders[1]`, etc.). Ver comentarios en `Encoder.cpp:82-85`.

### Odometria (`lib/Odometria/`)
Modelo diferencial usando solo ruedas traseras (td, ti).

| Parámetro | Valor |
|-----------|-------|
| Radio de rueda (`WHEEL_R_CM`) | 3.385 cm |
| Distancia entre ruedas (`TRACK_L_CM`) | 21.5 cm |
| Pulsos por vuelta (`PPR`) | 340 |

**Período de integración:** 1 ms (guarda por `micros()`)

**Modelo cinemático:**
```
dPulse_d = (td - tdAnt) / 2      # promedio ruedas traseras derechas
dPulse_i = (ti - tiAnt) / 2
delta_d  = 2π · dPulse_d / PPR   [rad]
s_d      = R · delta_d            [cm]
Δs       = (s_d + s_i) / 2
Δθ       = (s_d - s_i) / L
x  += Δs · cos(θ + Δθ/2)
y  += Δs · sin(θ + Δθ/2)
θ  += Δθ   (normalizado a (-π, π])
```

Log circular de 2000 muestras (`OdoSample { t_ms, x, y, th }`). Cuando llena, vuelca CSV por Serial:
```
t_ms,x,y,th
```

**Estado actual de main.cpp:** duplica la lógica de odometría inline Y hace `#include "Odometria.h"`. Hay conflicto de símbolos pendiente de resolver.

### Servos (`lib/Servos/`)
Control PWM via `ledcWrite` (ledc ESP32). Frecuencia 5 kHz, resolución 8 bits (0–255).

| Función | Canales | Pines |
|---------|---------|-------|
| `ruedaDelDcha(vel%)` | CANAL_7 (GPIO13) / CANAL_8 (GPIO12) | adelante/atrás |
| `ruedasIzda(vel%)` | CANAL_1 (GPIO17) / CANAL_2 (GPIO16) | adelante/atrás |
| `ruedaTrasDcha(vel%)` | CANAL_3 (GPIO27) / CANAL_4 (GPIO14) | adelante/atrás |

Velocidad en % (−100…+100 → duty 0–255). Pendiente: `ruedaTraslzda` no está implementada.

### Laser (`lib/Laser/`)
Dos sensores VL53L0X ToF en I2C (400 kHz).

| Parámetro | Valor |
|-----------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| XSHUT sensor 1 | GPIO 18 |
| XSHUT sensor 2 | GPIO 19 (**conflicto con pin MUX encoder**) |
| Dirección sensor 1 | 0x30 |
| Dirección sensor 2 | 0x31 |

`inicializar_LASER()` → apaga ambos, enciende uno a la vez para reasignar direcciones.
`leer_LASER(addr)` → distancia en mm, o -1 si error/fuera de rango.

**Conflicto de pines:** GPIO 19 usado como `pinMux` en Encoder Y como `LASER_XSHUT_2`. Requiere resolución antes de usar ambos módulos simultáneamente.

---

## Estado del proyecto (2026-04-29)

| Módulo | Estado |
|--------|--------|
| Encoder | Implementado, en prueba |
| Odometria | Implementado, módulo extraído de main.cpp |
| Servos | Implementado parcialmente (falta rueda trasera izquierda) |
| Laser | Implementado |
| LineFollower | STUB |
| PC (comunicación) | STUB |
| PS4 (mando) | STUB |

**main.cpp actual:** prueba de odometría — lee encoders, integra pose, loggea 2000 muestras y vuelca CSV por Serial. Necesita migrar a usar `Odometria.h` en lugar de la lógica inline duplicada.

---

## Conflictos / Issues conocidos

1. **GPIO 19 compartido** entre `pinMux` (Encoder) y `LASER_XSHUT_2` (Laser).
2. **Duplicado de lógica** en `main.cpp` vs `lib/Odometria/` — causará error de linking.
3. **`ruedaTraslzda()`** no implementada en Servos.
4. **`readW()` en Encoder.cpp** usa `680` como PPR (= 340×2) pero el divisor no está documentado como full-quad.
