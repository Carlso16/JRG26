# {JUAN RAMON JIMENEZ 26 - MARISCAL} — Robot autónomo 4x4 (ESP32 + 4 motores + encoders)

> Repositorio para el desarrollo de un robot móvil diferencial **4x4** basado en **ESP32**, con control de 4 motores independientes y estimación de estado (odometría) mediante encoders.  
> Incluye estructura modular, pruebas y (opcional) validación en simulación.

---

## 1) Objetivos del proyecto

- Control de movimiento: convertir consignas de **velocidad lineal `v`** y **velocidad angular `w`** en comandos de potencia/velocidad para **4 motores**.
- Lectura de encoders y cálculo de **velocidades de rueda**.
- **Odometría**: estimación en tiempo real de `(x, y, θ)` respecto al origen.
- Arquitectura de software robusta (FSM / tareas periódicas) y mantenible (PlatformIO).
- Base para comportamientos autónomos (seguimiento de trayectoria, navegación, slalom, etc.).

Referencia metodológica (odometría, setSpeed/readSpeed y pruebas de trayectorias): guiones de prácticas :contentReference[oaicite:0]{index=0}.  
Referencia entorno de simulación (si se usa) y flujo de pruebas: :contentReference[oaicite:1]{index=1}.

---

## 2) Hardware (resumen)

### 2.1 Plataforma
- MCU: **ESP32**
- Tracción: **diferencial 4x4**
  - Lado izquierdo: 2 motores (delantero + trasero)
  - Lado derecho: 2 motores (delantero + trasero)

### 2.2 Sensores
- Encoders: **sí** (sin driver dedicado; lectura directa por GPIO/PCNT o interrupciones)

### 2.3 Actuadores
- 4 motores DC (o equivalentes) con sus drivers (H-bridge / ESC / drivers PWM)

### 2.4 Parámetros mecánicos (rellenar)
- Radio de rueda `R`: `___` m
- Distancia entre ruedas (track width) `L`: `___` m
- Relación reductora `G`: `___`
- Ticks por vuelta del encoder (en eje medido) `N`: `___` ticks/rev
- Montaje: diámetro rueda, ancho, masa, reparto de pesos: `___`

---

## 3) Software

### 3.1 Toolchain
- **PlatformIO**
- Framework: Arduino-ESP32 (o ESP-IDF si se migra)
- Lenguaje: C++

### 3.2 Convenciones
- `src/` solo lógica de aplicación (máquina de estados / control).
- `lib/` para módulos: motores, encoders, odometría, filtros, etc.
- Sin lógica “grande” en `main.cpp`: solo inicialización + bucle de scheduling/FSM.

---

## 4) Estructura del repositorio (propuesta)
    ├─ platformio.ini
├─ README.md
├─ src/
│ ├─ main.cpp
│ ├─ App.cpp
│ └─ App.h
├─ lib/
│ ├─ MotorDriver/
│ │ ├─ MotorDriver.h
│ │ └─ MotorDriver.cpp
│ ├─ Encoder/
│ │ ├─ Encoder.h
│ │ └─ Encoder.cpp
│ ├─ Kinematics/
│ │ ├─ DiffDriveKinematics.h
│ │ └─ DiffDriveKinematics.cpp
│ ├─ Odometry/
│ │ ├─ Odometry.h
│ │ └─ Odometry.cpp
│ ├─ Control/
│ │ ├─ SpeedController.h
│ │ └─ SpeedController.cpp
│ └─ Utils/
│ ├─ Timebase.h
│ └─ Filters.h
├─ test/ # unit/integration tests (si aplica)
└─ docs/
├─ wiring.md
├─ params.md
└─ logs/

---

## 5) Fundamento teórico (mínimo)

### 5.1 Cinemática diferencial (velocidades)
Para un robot diferencial (equivalente aunque haya 4 motores, agrupando por lado):

- Velocidad lineal:
\[
v = \frac{R}{2}(\omega_R + \omega_L)
\]
- Velocidad angular:
\[
w = \frac{R}{L}(\omega_R - \omega_L)
\]

De donde:
\[
\omega_R = \frac{v}{R} + \frac{L}{2R}w,\quad
\omega_L = \frac{v}{R} - \frac{L}{2R}w
\]

En 4x4:
- `ω_L` se aplica a **motor delantero izq** y **trasero izq**
- `ω_R` se aplica a **motor delantero der** y **trasero der**
- Si hay diferencias mecánicas, se introduce calibración por motor (ganancias).

### 5.2 Encoder → velocidad rueda
Si `Δticks` en un periodo `Δt`:

- Ángulo rueda:
\[
\Delta\phi = 2\pi \frac{\Delta ticks}{N}
\]
- Velocidad angular:
\[
\omega = \frac{\Delta\phi}{\Delta t}
\]
- Distancia:
\[
\Delta s = R\Delta\phi
\]

### 5.3 Odometría (integración incremental)
Con incrementos de distancia izquierda/derecha `Δs_L`, `Δs_R`:

\[
\Delta s = \frac{\Delta s_R + \Delta s_L}{2},\quad
\Delta \theta = \frac{\Delta s_R - \Delta s_L}{L}
\]

Actualización (aprox. de punto medio):
\[
x_{k+1}=x_k+\Delta s\cos(\theta_k+\Delta\theta/2)
\]
\[
y_{k+1}=y_k+\Delta s\sin(\theta_k+\Delta\theta/2)
\]
\[
\theta_{k+1}=\theta_k+\Delta\theta
\]

Las tareas “setSpeed/readSpeed” y “updateOdometry/readOdometry” y sus verificaciones prácticas (recta, giro puro, plot) siguen el esquema indicado en el guion :contentReference[oaicite:2]{index=2}.

---

## 6) Cómo compilar y flashear

1. Instalar PlatformIO (VSCode).
2. Conectar ESP32 por USB.
3. Configurar `platformio.ini` (board, monitor_speed, etc.).
4. Compilar:
   - `pio run`
5. Subir firmware:
   - `pio run -t upload`
6. Monitor serie:
   - `pio device monitor`

---

## 7) Configuración (parámetros)

Archivo recomendado: `docs/params.md` o `lib/Utils/Config.h`.

- `WHEEL_RADIUS_M`
- `TRACK_WIDTH_M`
- `ENC_TICKS_PER_REV`
- `CONTROL_DT_S` (periodo control)
- `ODOM_DT_S` (periodo odometría)
- `PWM_FREQ`, `PWM_RES_BITS`
- `MOTOR_POLARITY_*` (signos)
- Calibración por motor: `k_ff`, `k_p`, `k_i` (si hay PI)

---

## 8) Arquitectura de control (resumen)

### 8.1 Capas recomendadas
- **Drivers**: PWM, dirección, lectura encoder.
- **Estimación**: velocidad ruedas + odometría.
- **Control**:
  - Abierto: `PWM = f(ω_ref)` (feedforward + saturación)
  - Cerrado: PI por rueda (si se implementa)
- **Comportamiento**: FSM (manual/autónomo/pruebas) y generación de consignas `v,w`.

### 8.2 Tareas periódicas sugeridas
- `MotorControlTask` a 100–200 Hz
- `OdometryTask` a 50–100 Hz
- `TelemetryTask` a 10–20 Hz (serial/logs)

---

## 9) Pruebas y validación

### 9.1 Pruebas mínimas (robot real)
- **Giro puro**: `setSpeed(0, w)` ⇒ cambia `θ` principalmente.
- **Recta**: `setSpeed(v, 0)` ⇒ trayectoria recta, cambia `x,y`.
- **Distancia conocida**: avanzar 0.40 m (baldosa ~40 cm) por tiempo u odometría :contentReference[oaicite:3]{index=3}.
- Guardar log CSV de `(t, x, y, θ, v, w, ticks...)` y plottear.

### 9.2 Simulación (opcional)
Si se usa un simulador, documentar escena, versión y cómo ejecutar (en las prácticas se usa CoppeliaSim como referencia de flujo) :contentReference[oaicite:4]{index=4}.

---

## 10) Telemetría y logs

- Formato CSV recomendado:
  - `t, x, y, th, v_cmd, w_cmd, wL, wR, pwmL, pwmR, ticksL, ticksR`
- Niveles de log: `INFO/WARN/ERROR`
- Considerar “downsampling” para no saturar el puerto serie.

---

## 11) Seguridad y límites

- Saturación de consignas:
  - `|v| ≤ V_MAX`, `|w| ≤ W_MAX`
- Watchdog / timeout:
  - Si no hay comandos en `T_cmd_timeout` ⇒ parar motores.
- E-stop (si hay botón): prioridad máxima.

---

## 12) Apartado (EN BLANCO) — Explicación de funciones

> **Rellenar aquí** la explicación de cómo se usan y qué hace cada función/clase del proyecto.

### 12.1 `setSpeed(v, w)`
- 

### 12.2 `readSpeed()`
- 

### 12.3 `updateOdometry()`
- 

### 12.4 `readOdometry()`
- 

### 12.5 FSM (estados y transiciones)
- 

---

## 13) Roadmap

- [ ] Lectura robusta de encoders (PCNT / interrupciones con anti-rebote)
- [ ] Estimación de velocidad por filtro (media móvil / IIR)
- [ ] Control PI por rueda + anti-windup
- [ ] Calibración automática (ganancias por lado)
- [ ] Seguimiento de trayectoria (slalom / waypoint)
- [ ] Fusión con IMU (si se añade): complementar `θ` :contentReference[oaicite:5]{index=5}

---

## 14) Licencia
Elegir una:
- MIT / Apache-2.0 / GPL-3.0

---

## 15) Autores
- {SABROSO MELON}
- {colaboradores}
