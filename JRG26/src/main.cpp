#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>
#include <Preferences.h>

#include "Laser_2.h"
#include "Servos.h"

// ============================================================
// WIFI / SERVIDOR WEB
// ============================================================

const char* WIFI_SSID = "SUMO_ROBOT";
const char* WIFI_PASS = "12345678";   // minimo 8 caracteres para SoftAP

WebServer server(80);
Preferences prefs;

#define NVS_NAMESPACE "sumo_cfg"
#define CONFIG_VERSION 1

// ============================================================
// VELOCIDADES DEL ROBOT
// ============================================================
volatile int16_t V_BUSCA_1_DCHA   = 25;
volatile int16_t V_BUSCA_1_IZDA   = -25;

volatile int16_t V_BUSCA_2_DCHA   = 25;
volatile int16_t V_BUSCA_2_IZDA   = -25;

volatile int16_t V_BUSCA_3_DCHA   = -15;
volatile int16_t V_BUSCA_3_IZDA   = 15;

volatile int16_t V_ATACA_DCHA     = 20;
volatile int16_t V_ATACA_IZDA     = 20;

volatile int16_t V_RETROCEDE_DCHA = -20;
volatile int16_t V_RETROCEDE_IZDA = -20;

// ============================================================
// PINES
// ============================================================
#define PIN_LED 2

// ============================================================
// MOTORES
// ============================================================
int CONSIGNA_DCHA = 0;
int CONSIGNA_IZDA = 0;

// ============================================================
// TIEMPOS MEF
// ============================================================
uint32_t t0 = 0;
uint32_t t_atacado   = 0;
uint32_t T_ATAQUE    = 1288;
uint32_t T_RETROCESO = 1288;
uint32_t T_REPOSO    = 800;
uint32_t T_fin_busqueda = 4000;
uint32_t T_TRAN      = 100;

// ============================================================
// PERIODOS TICKER
// ============================================================
uint32_t TICKER_LASER_MS   = 50;
const uint32_t TICKER_MOTORES_MS = 10;
uint32_t TICKER_MEF_MS     = 100;

// ============================================================
// CARGA DEL SERVIDOR WEB
// ============================================================
const uint32_t SERVER_HANDLE_MS = 40;
const uint32_t WEB_STATUS_REFRESH_MS = 1000;

// ============================================================
// LASER
// ============================================================
#define DIR_LASER 0x29

volatile int16_t laser = -1;
int16_t dist_activacion = 800;
volatile bool laser_detectado = false;
volatile int16_t distancia_inicio_ataque_mm = -1;

// ============================================================
// CONTROL GENERAL
// ============================================================
volatile bool robot_habilitado = false;
volatile bool flag_laser = false;

// ============================================================
// TICKERS
// ============================================================
Ticker ticker_laser;
Ticker ticker_motores;
Ticker ticker_mef;

// ============================================================
// ESTADOS
// ============================================================
typedef enum {
  REPOSO,
  BUSCA_1,
  BUSCA_2,
  BUSCA_3,
  TRANSIATACA,
  ATACA,
  RETROCEDE,
  E6,
  E7,
  E8
} Estado_t;

volatile Estado_t estado_actual = ATACA;

// ============================================================
// VALORES POR DEFECTO DE CONFIGURACION
// ============================================================
const int16_t DEF_V_BUSCA_1_DCHA   = 25;
const int16_t DEF_V_BUSCA_1_IZDA   = -25;

const int16_t DEF_V_BUSCA_2_DCHA   = 25;
const int16_t DEF_V_BUSCA_2_IZDA   = -25;

const int16_t DEF_V_BUSCA_3_DCHA   = -15;
const int16_t DEF_V_BUSCA_3_IZDA   = 15;

const int16_t DEF_V_ATACA_DCHA     = 20;
const int16_t DEF_V_ATACA_IZDA     = 20;

const int16_t DEF_V_RETROCEDE_DCHA = -20;
const int16_t DEF_V_RETROCEDE_IZDA = -20;
const uint32_t DEF_T_REPOSO        = 800;
const uint32_t DEF_T_ATAQUE        = 1288;
const uint32_t DEF_T_RETROCESO     = 1288;
const uint32_t DEF_T_fin_busqueda  = 4000;
const uint32_t DEF_T_TRAN          = 100;

const uint32_t DEF_TICKER_LASER_MS = 50;
const uint32_t DEF_TICKER_MEF_MS   = 100;

const int16_t DEF_DIST_ACTIVACION  = 800;
const float DEF_KP    = 0.25f;
const float DEF_KI    = 10.0f;
const float DEF_VMAX  = 11.0f;

// ============================================================
// PROTOTIPOS
// ============================================================
void initSUMO();
void init_wifi_ap();
void init_servidor_web();

void aplicar_configuracion_por_defecto();
void cargar_configuracion();
void guardar_configuracion();
void restaurar_configuracion_por_defecto();

void reprogramar_ticker_laser();
void reprogramar_ticker_mef();

void cb_ticker_laser();
void atender_laser_pendiente();
void atender_servidor_wifi();

int16_t leer_laser();
void actualizar_laser();
void limpiar_distancia_inicio_ataque();
void registrar_distancia_inicio_ataque();

void mover_robot(float velocidadDcha, float velocidadIzda);
void aplicar_MEF();
void aplicar_motores();

void parar_robot_logico();
void mandar_robot_a_ataca();
void resetear_MEF();
const char* estadoToTexto(Estado_t estado);

String paginaHTML();
void handleRoot();
void handleStatus();
void handleConfig();
void handleStart();
void handleStop();
void handleReset();
void handleSaveConfig();
void handleDefaultConfig();
void handleNotFound();

uint32_t leerArgU32(const char* nombre, uint32_t actual, uint32_t minimo, uint32_t maximo);
int16_t leerArgI16(const char* nombre, int16_t actual, int16_t minimo, int16_t maximo);
float leerArgFloat(const char* nombre, float actual, float minimo, float maximo);
void enviarJSONStatus();

// ============================================================
// SETUP & LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n--- Inicializando robot ---");
  initSUMO();

  Serial.println("Inicializacion completada.");
  Serial.println("Conectate al WiFi SUMO_ROBOT / 12345678");
  Serial.print("Abre: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  atender_laser_pendiente();
  atender_servidor_wifi();
}

void atender_servidor_wifi() {
  static uint32_t t_server = 0;
  uint32_t ahora = millis();
  if (ahora - t_server >= SERVER_HANDLE_MS) {
    t_server = ahora;
    server.handleClient();
  }
}

// ============================================================
// INIT GENERAL
// ============================================================
void initSUMO() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  aplicar_configuracion_por_defecto();
  cargar_configuracion();

  robot_habilitado = false;
  estado_actual = ATACA; 
  t0 = 0;
  t_atacado = 0;
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;
  if (!inicializar_LASER()) {
    Serial.println("ERROR: inicializar_LASER() fallo. Revisa cableado/XSHUT/I2C.");
    while (1) {
      delay(1000);
    }
  }

  initPWM(TICKER_MOTORES_MS);
  init_wifi_ap();
  init_servidor_web();

  ticker_motores.attach_ms(TICKER_MOTORES_MS, aplicar_motores);
  ticker_mef.attach_ms(TICKER_MEF_MS, aplicar_MEF);

  reprogramar_ticker_laser();
}

void init_wifi_ap() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
}

void init_servidor_web() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/config", HTTP_GET, handleConfig);
  server.on("/api/start", HTTP_GET, handleStart);
  server.on("/api/stop", HTTP_GET, handleStop);
  server.on("/api/reset", HTTP_GET, handleReset);
  server.on("/api/save_config", HTTP_GET, handleSaveConfig);
  server.on("/api/default_config", HTTP_GET, handleDefaultConfig);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ============================================================
// CONFIGURACION PERSISTENTE NVS
// ============================================================
void aplicar_configuracion_por_defecto() {
  V_BUSCA_1_DCHA   = DEF_V_BUSCA_1_DCHA;
  V_BUSCA_1_IZDA   = DEF_V_BUSCA_1_IZDA;
  V_BUSCA_2_DCHA   = DEF_V_BUSCA_2_DCHA;
  V_BUSCA_2_IZDA   = DEF_V_BUSCA_2_IZDA;
  V_BUSCA_3_DCHA   = DEF_V_BUSCA_3_DCHA;
  V_BUSCA_3_IZDA   = DEF_V_BUSCA_3_IZDA;
  V_ATACA_DCHA     = DEF_V_ATACA_DCHA;
  V_ATACA_IZDA     = DEF_V_ATACA_IZDA;
  V_RETROCEDE_DCHA = DEF_V_RETROCEDE_DCHA;
  V_RETROCEDE_IZDA = DEF_V_RETROCEDE_IZDA;
  T_REPOSO       = DEF_T_REPOSO;
  T_ATAQUE       = DEF_T_ATAQUE;
  T_RETROCESO    = DEF_T_RETROCESO;
  T_fin_busqueda = DEF_T_fin_busqueda;
  T_TRAN         = DEF_T_TRAN;
  TICKER_LASER_MS = DEF_TICKER_LASER_MS;
  TICKER_MEF_MS   = DEF_TICKER_MEF_MS;
  dist_activacion = DEF_DIST_ACTIVACION;

  setConstantesControl(DEF_KP, DEF_KI, DEF_VMAX, true);
}

void cargar_configuracion() {
  prefs.begin(NVS_NAMESPACE, true);
  uint32_t version = prefs.getUInt("version", 0);

  if (version != CONFIG_VERSION) {
    prefs.end();
    Serial.println("No hay configuracion NVS valida. Usando valores por defecto.");
    return;
  }

  TICKER_LASER_MS = prefs.getUInt("tl", DEF_TICKER_LASER_MS);
  TICKER_MEF_MS   = prefs.getUInt("tf", DEF_TICKER_MEF_MS);
  dist_activacion = prefs.getShort("umbral", DEF_DIST_ACTIVACION);

  T_REPOSO       = prefs.getUInt("reposo", DEF_T_REPOSO);
  T_ATAQUE       = prefs.getUInt("ataque", DEF_T_ATAQUE);
  T_RETROCESO    = prefs.getUInt("retro", DEF_T_RETROCESO);
  T_fin_busqueda = prefs.getUInt("finbus", DEF_T_fin_busqueda);
  T_TRAN         = prefs.getUInt("ttran", DEF_T_TRAN);
  V_BUSCA_1_DCHA   = prefs.getShort("vb1_d", DEF_V_BUSCA_1_DCHA);
  V_BUSCA_1_IZDA   = prefs.getShort("vb1_i", DEF_V_BUSCA_1_IZDA);
  V_BUSCA_2_DCHA   = prefs.getShort("vb2_d", DEF_V_BUSCA_2_DCHA);
  V_BUSCA_2_IZDA   = prefs.getShort("vb2_i", DEF_V_BUSCA_2_IZDA);
  V_BUSCA_3_DCHA   = prefs.getShort("vb3_d", DEF_V_BUSCA_3_DCHA);
  V_BUSCA_3_IZDA   = prefs.getShort("vb3_i", DEF_V_BUSCA_3_IZDA);
  V_ATACA_DCHA     = prefs.getShort("va_d", DEF_V_ATACA_DCHA);
  V_ATACA_IZDA     = prefs.getShort("va_i", DEF_V_ATACA_IZDA);
  V_RETROCEDE_DCHA = prefs.getShort("vr_d", DEF_V_RETROCEDE_DCHA);
  V_RETROCEDE_IZDA = prefs.getShort("vr_i", DEF_V_RETROCEDE_IZDA);

  float kp   = prefs.getFloat("kp", DEF_KP);
  float ki   = prefs.getFloat("ki", DEF_KI);
  float vmax = prefs.getFloat("vmax", DEF_VMAX);
  prefs.end();

  TICKER_LASER_MS = constrain(TICKER_LASER_MS, 50UL, 1000UL); 
  TICKER_MEF_MS   = constrain(TICKER_MEF_MS, 10UL, 1000UL);
  dist_activacion = constrain(dist_activacion, 20, 4000);

  T_REPOSO       = constrain(T_REPOSO, 0UL, 20000UL);
  T_ATAQUE       = constrain(T_ATAQUE, 0UL, 20000UL);
  T_RETROCESO    = constrain(T_RETROCESO, 0UL, 20000UL);
  T_fin_busqueda = constrain(T_fin_busqueda, 0UL, 60000UL);
  T_TRAN         = constrain(T_TRAN, 0UL, 20000UL);
  V_BUSCA_1_DCHA   = constrain(V_BUSCA_1_DCHA, -100, 100);
  V_BUSCA_1_IZDA   = constrain(V_BUSCA_1_IZDA, -100, 100);
  V_BUSCA_2_DCHA   = constrain(V_BUSCA_2_DCHA, -100, 100);
  V_BUSCA_2_IZDA   = constrain(V_BUSCA_2_IZDA, -100, 100);
  V_BUSCA_3_DCHA   = constrain(V_BUSCA_3_DCHA, -100, 100);
  V_BUSCA_3_IZDA   = constrain(V_BUSCA_3_IZDA, -100, 100);
  V_ATACA_DCHA     = constrain(V_ATACA_DCHA, -100, 100);
  V_ATACA_IZDA     = constrain(V_ATACA_IZDA, -100, 100);
  V_RETROCEDE_DCHA = constrain(V_RETROCEDE_DCHA, -100, 100);
  V_RETROCEDE_IZDA = constrain(V_RETROCEDE_IZDA, -100, 100);

  kp   = constrain(kp, 0.0f, 5.0f);
  ki   = constrain(ki, 0.0f, 100.0f);
  vmax = constrain(vmax, 1.0f, 20.0f);

  setConstantesControl(kp, ki, vmax, true);
  Serial.println("Configuracion cargada desde NVS.");
}

void guardar_configuracion() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putUInt("version", CONFIG_VERSION);

  prefs.putUInt("tl", TICKER_LASER_MS);
  prefs.putUInt("tf", TICKER_MEF_MS);
  prefs.putShort("umbral", dist_activacion);

  prefs.putUInt("reposo", T_REPOSO);
  prefs.putUInt("ataque", T_ATAQUE);
  prefs.putUInt("retroceso", T_RETROCESO);
  prefs.putUInt("finbus", T_fin_busqueda);
  prefs.putUInt("ttran", T_TRAN);

  prefs.putShort("vb1_d", V_BUSCA_1_DCHA);
  prefs.putShort("vb1_i", V_BUSCA_1_IZDA);
  prefs.putShort("vb2_d", V_BUSCA_2_DCHA);
  prefs.putShort("vb2_i", V_BUSCA_2_IZDA);
  prefs.putShort("vb3_d", V_BUSCA_3_DCHA);
  prefs.putShort("vb3_i", V_BUSCA_3_IZDA);
  prefs.putShort("va_d", V_ATACA_DCHA);
  prefs.putShort("va_i", V_ATACA_IZDA);
  prefs.putShort("vr_d", V_RETROCEDE_DCHA);
  prefs.putShort("vr_i", V_RETROCEDE_IZDA);

  prefs.putFloat("kp", getKpControl());
  prefs.putFloat("ki", getKiControl());
  prefs.putFloat("vmax", getVmaxControl());
  prefs.end();

  Serial.println("Configuracion guardada en NVS.");
}

void restaurar_configuracion_por_defecto() {
  aplicar_configuracion_por_defecto();
  reprogramar_ticker_laser();
  reprogramar_ticker_mef();
  guardar_configuracion();

  robot_habilitado = false;
  resetear_MEF();
}

// ============================================================
// TICKERS
// ============================================================
void reprogramar_ticker_laser() {
  ticker_laser.detach();
  TICKER_LASER_MS = constrain(TICKER_LASER_MS, 50UL, 1000UL);
  ticker_laser.attach_ms(TICKER_LASER_MS, cb_ticker_laser);
}

void reprogramar_ticker_mef() {
  ticker_mef.detach();
  TICKER_MEF_MS = constrain(TICKER_MEF_MS, 10UL, 1000UL);
  ticker_mef.attach_ms(TICKER_MEF_MS, aplicar_MEF);
}

void cb_ticker_laser() {
  flag_laser = true;
}

void atender_laser_pendiente() {
  if (!flag_laser) return;
  flag_laser = false;
  actualizar_laser();
}

// ============================================================
// MEF
// ============================================================
void aplicar_MEF() {
  if (!robot_habilitado) {
    CONSIGNA_DCHA = 0;
    CONSIGNA_IZDA = 0;
    return;
  }

  switch (estado_actual) {
    case ATACA:
      if (t0 == 0) t0 = millis();
      
      CONSIGNA_DCHA = V_ATACA_DCHA;
      CONSIGNA_IZDA = V_ATACA_IZDA;

      if (millis() - t0 >= T_ATAQUE) {
        estado_actual = RETROCEDE;
        t0 = millis();
      }
      break;

    case RETROCEDE:
      if (millis() - t0 >= T_RETROCESO) {
        limpiar_distancia_inicio_ataque();
        // Flujo restaurado: vuelve a buscar con el láser tras el ataque
        estado_actual = BUSCA_1;
        t_atacado = 0;
        t0 = millis();
      }
      CONSIGNA_DCHA = V_RETROCEDE_DCHA;
      CONSIGNA_IZDA = V_RETROCEDE_IZDA;
      break;

    case REPOSO:
      if (t0 == 0) t0 = millis();
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;
      if (millis() - t0 >= T_REPOSO) {
        limpiar_distancia_inicio_ataque();
        estado_actual = BUSCA_1;
        t0 = millis();
      }
      break;

    case BUSCA_1:
      if (laser_detectado) {
        estado_actual = BUSCA_2;
        t0 = millis();
      }
      CONSIGNA_DCHA = V_BUSCA_1_DCHA;
      CONSIGNA_IZDA = V_BUSCA_1_IZDA;
      break;

    case BUSCA_2:
      if (!laser_detectado) {
        estado_actual = BUSCA_3;
        t0 = millis();
      }
      CONSIGNA_DCHA = V_BUSCA_2_DCHA;
      CONSIGNA_IZDA = V_BUSCA_2_IZDA;
      break;

    case BUSCA_3:
      if (laser_detectado) {
        registrar_distancia_inicio_ataque();
        estado_actual = TRANSIATACA;
        t0 = millis();
      }
      else if (millis() - t0 >= T_fin_busqueda) {
        limpiar_distancia_inicio_ataque();
        estado_actual = BUSCA_1;
        t0 = millis();
      }
      CONSIGNA_DCHA = V_BUSCA_3_DCHA;
      CONSIGNA_IZDA = V_BUSCA_3_IZDA;
      break;

    case TRANSIATACA:
      if (millis() - t0 >= T_TRAN) {
        estado_actual = ATACA;
        t0 = millis();
      }
      break;

    default:
      limpiar_distancia_inicio_ataque();
      estado_actual = BUSCA_1;
      t0 = millis();
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;
      break;
  }
}

// ============================================================
// LASER & MOTORES
// ============================================================
void actualizar_laser() {
  laser = leer_LASER(DIR_LASER);
  laser_detectado = (laser >= 0 && laser <= dist_activacion);
  digitalWrite(PIN_LED, laser_detectado ? HIGH : LOW);
}

int16_t leer_laser() {
  return leer_LASER(DIR_LASER);
}

void limpiar_distancia_inicio_ataque() {
  distancia_inicio_ataque_mm = -1;
}

void registrar_distancia_inicio_ataque() {
  distancia_inicio_ataque_mm = -1;
  if (laser >= 0) {
    distancia_inicio_ataque_mm = laser;
  }
}

void aplicar_motores() {
  mover_robot(CONSIGNA_DCHA, CONSIGNA_IZDA);
}

void mover_robot(float velocidadDcha, float velocidadIzda) {
  wRuedas(velocidadDcha, velocidadDcha, velocidadIzda);
}

void parar_robot_logico() {
  robot_habilitado = false;
  mandar_robot_a_ataca();
}

void mandar_robot_a_ataca() {
  estado_actual = ATACA; 
  t0 = millis();
  t_atacado = 0;
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;
}

void resetear_MEF() {
  limpiar_distancia_inicio_ataque();
  estado_actual = ATACA; 
  t0 = 0;
  t_atacado = 0;
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;
}

// ============================================================
// SERVIDOR WEB
// ============================================================
String paginaHTML() {
  String html;
  html.reserve(11000);

  html += F("<!doctype html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>SUMO Robot</title>");
  html += F("<style>");
  html += F("body{font-family:Arial;margin:20px;max-width:900px}");
  html += F("input{width:90px}button{margin:4px;padding:8px}");
  html += F(".ok{color:green}.bad{color:red}");
  html += F(".box{border:1px solid #ccc;padding:12px;margin:10px 0;border-radius:8px}");
  html += F("label{display:inline-block;min-width:235px;margin:5px 0}");
  html += F(".fila{display:block;margin:3px 0}");
  html += F("</style></head><body>");
  html += F("<h2>Configuracion robot SUMO</h2>");

  html += F("<div class='box'><h3>Estado actual</h3>");
  html += F("<p>Robot: <b id='robot'>-</b></p>");
  html += F("<p>Estado MEF: <b id='estado'>-</b></p>");
  html += F("<p>Laser detectado: <b id='laser_detectado'>-</b></p>");
  html += F("<p>Distancia laser: <b id='laser'>-</b> mm</p>");
  html += F("<p>Distancia cambio a ataque: <b id='laser_ataque'>None</b></p>");
  html += F("<p>Consignas: DCHA <b id='cd'>-</b> | IZDA <b id='ci'>-</b></p>");
  html += F("<p>Control: Kp <b id='kp_estado'>-</b> | Ki <b id='ki_estado'>-</b> | Vmax <b id='vmax_estado'>-</b></p>");
  html += F("<button onclick='startRobot()'>START</button>");
  html += F("<button onclick='stopRobot()'>STOP</button>");
  html += F("<button onclick='resetRobot()'>RESET MEF</button>");
  html += F("</div>");

  html += F("<div class='box'><h3>Configurar</h3>");

  html += F("<h4>Tickers [ms]</h4>");
  html += F("<label>ticker_laser</label><input id='tl' type='number' min='50' max='1000'><br>"); 
  html += F("<label>ticker_mef</label><input id='tf' type='number' min='10' max='1000'><br>");
  html += F("<p>ticker_motores fijo: <b id='tm'>10</b> ms</p>");

  html += F("<h4>Control PI motores</h4>");
  html += F("<label>Kp</label><input id='kp' type='number' step='0.01' min='0' max='5'><br>");
  html += F("<label>Ki</label><input id='ki' type='number' step='0.1' min='0' max='100'><br>");
  html += F("<label>Vmax salida PI</label><input id='vmax' type='number' step='0.1' min='1' max='20'><br>");
  html += F("<h4>Laser</h4>");
  html += F("<label>Umbral deteccion laser [mm]</label><input id='umbral' type='number' min='20' max='4000'><br>");

  html += F("<h4>Tiempos MEF [ms]</h4>");
  html += F("<label>T_REPOSO</label><input id='reposo' type='number' min='0' max='20000'><br>");
  html += F("<label>T_TRAN (Transicion Ataque)</label><input id='t_tran' type='number' min='0' max='20000'><br>");
  html += F("<label>T_ATAQUE</label><input id='ataque' type='number' min='0' max='20000'><br>");
  html += F("<label>T_RETROCESO</label><input id='retroceso' type='number' min='0' max='20000'><br>");
  html += F("<label>T_FIN_BUSQUEDA</label><input id='fin_busqueda' type='number' min='0' max='60000'><br>");

  html += F("<h4>Velocidades [-100 a 100]</h4>");
  html += F("<div class='fila'><label>BUSCA_1 DCHA / IZDA</label><input id='v_b1_d' type='number' min='-100' max='100'> <input id='v_b1_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>BUSCA_2 DCHA / IZDA</label><input id='v_b2_d' type='number' min='-100' max='100'> <input id='v_b2_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>BUSCA_3 DCHA / IZDA</label><input id='v_b3_d' type='number' min='-100' max='100'> <input id='v_b3_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>ATACA DCHA / IZDA</label><input id='v_ataca_d' type='number' min='-100' max='100'> <input id='v_ataca_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>RETROCEDE DCHA / IZDA</label><input id='v_retro_d' type='number' min='-100' max='100'> <input id='v_retro_i' type='number' min='-100' max='100'></div>");
  html += F("<br><button onclick='aplicar()'>Aplicar cambios</button>");
  html += F("<button onclick='guardarMemoria()'>Guardar en memoria</button>");
  html += F("<button onclick='restaurarDefecto()'>Restaurar defecto</button>");
  html += F("<p id='msg'></p>");
  html += F("</div>");

  html += F("<script>");
  html += F("let primera=true;");
  html += F("const campos=['tl','tf','kp','ki','vmax','umbral','reposo','t_tran','ataque','retroceso','fin_busqueda','v_b1_d','v_b1_i','v_b2_d','v_b2_i','v_b3_d','v_b3_i','v_ataca_d','v_ataca_i','v_retro_d','v_retro_i'];");
  html += F("function txt(b){return b?'SI':'NO'}");
  html += F("function cls(id,b){let e=document.getElementById(id);e.className=b?'ok':'bad'}");
  html += F("async function estado(){let r=await fetch('/api/status');let s=await r.json();");
  html += F("document.getElementById('robot').innerText=s.robot_habilitado?'HABILITADO':'PARADO';cls('robot',s.robot_habilitado);");
  html += F("document.getElementById('estado').innerText=s.estado;");
  html += F("document.getElementById('laser_detectado').innerText=txt(s.laser_detectado);cls('laser_detectado',s.laser_detectado);");
  html += F("document.getElementById('laser').innerText=s.laser_mm;");
  html += F("document.getElementById('laser_ataque').innerText=(s.laser_ataque_mm===null?'None':s.laser_ataque_mm+' mm');");
  html += F("document.getElementById('cd').innerText=s.consigna_dcha;");
  html += F("document.getElementById('ci').innerText=s.consigna_izda;");
  html += F("document.getElementById('tm').innerText=s.tm;");
  html += F("document.getElementById('kp_estado').innerText=s.kp;");
  html += F("document.getElementById('ki_estado').innerText=s.ki;");
  html += F("document.getElementById('vmax_estado').innerText=s.vmax;");
  html += F("if(primera){campos.forEach(k=>{if(document.getElementById(k)){document.getElementById(k).value=s[k];}});primera=false;}}");
  html += F("async function aplicar(){let p=new URLSearchParams();campos.forEach(k=>p.append(k,document.getElementById(k).value));let r=await fetch('/api/config?'+p.toString());document.getElementById('msg').innerText=await r.text();primera=true;estado();}");
  html += F("async function guardarMemoria(){await aplicar();let r=await fetch('/api/save_config');document.getElementById('msg').innerText=await r.text();primera=true;estado();}");
  html += F("async function restaurarDefecto(){let r=await fetch('/api/default_config');document.getElementById('msg').innerText=await r.text();primera=true;estado();}");
  html += F("async function startRobot(){await fetch('/api/start');estado();}");
  html += F("async function stopRobot(){await fetch('/api/stop');estado();}");
  html += F("async function resetRobot(){await fetch('/api/reset');estado();}");
  html += F("setInterval(estado,");
  html += String(WEB_STATUS_REFRESH_MS);
  html += F(");estado();");
  html += F("</script></body></html>");
  return html;
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", paginaHTML());
}

void handleStatus() {
  enviarJSONStatus();
}

void handleConfig() {
  TICKER_LASER_MS = leerArgU32("tl", TICKER_LASER_MS, 50, 1000); 
  TICKER_MEF_MS   = leerArgU32("tf", TICKER_MEF_MS, 10, 1000);

  float kp_actual = getKpControl();
  float ki_actual = getKiControl();
  float vmax_actual = getVmaxControl();

  float kp = leerArgFloat("kp", kp_actual, 0.0f, 5.0f);
  float ki = leerArgFloat("ki", ki_actual, 0.0f, 100.0f);
  float vmax = leerArgFloat("vmax", vmax_actual, 1.0f, 20.0f);
  bool cambio_control = (kp != kp_actual) || (ki != ki_actual) || (vmax != vmax_actual);
  setConstantesControl(kp, ki, vmax, cambio_control);
  dist_activacion = leerArgI16("umbral", dist_activacion, 20, 4000);

  T_REPOSO    = leerArgU32("reposo", T_REPOSO, 0, 20000);
  T_TRAN      = leerArgU32("t_tran", T_TRAN, 0, 20000);
  T_ATAQUE    = leerArgU32("ataque", T_ATAQUE, 0, 20000);
  T_RETROCESO    = leerArgU32("retroceso", T_RETROCESO, 0, 20000);
  T_fin_busqueda = leerArgU32("fin_busqueda", T_fin_busqueda, 0, 60000);

  V_BUSCA_1_DCHA   = leerArgI16("v_b1_d", V_BUSCA_1_DCHA, -100, 100);
  V_BUSCA_1_IZDA   = leerArgI16("v_b1_i", V_BUSCA_1_IZDA, -100, 100);
  V_BUSCA_2_DCHA   = leerArgI16("v_b2_d", V_BUSCA_2_DCHA, -100, 100);
  V_BUSCA_2_IZDA   = leerArgI16("v_b2_i", V_BUSCA_2_IZDA, -100, 100);
  V_BUSCA_3_DCHA   = leerArgI16("v_b3_d", V_BUSCA_3_DCHA, -100, 100);
  V_BUSCA_3_IZDA   = leerArgI16("v_b3_i", V_BUSCA_3_IZDA, -100, 100);
  V_ATACA_DCHA     = leerArgI16("v_ataca_d", V_ATACA_DCHA, -100, 100);
  V_ATACA_IZDA     = leerArgI16("v_ataca_i", V_ATACA_IZDA, -100, 100);
  V_RETROCEDE_DCHA = leerArgI16("v_retro_d", V_RETROCEDE_DCHA, -100, 100);
  V_RETROCEDE_IZDA = leerArgI16("v_retro_i", V_RETROCEDE_IZDA, -100, 100);

  reprogramar_ticker_laser();
  reprogramar_ticker_mef();

  server.send(200, "text/plain", "Configuracion aplicada en RAM");
}

void handleStart() {
  robot_habilitado = true;
  mandar_robot_a_ataca(); 
  server.send(200, "text/plain", "Robot habilitado; enviado a ATACA");
}

void handleStop() {
  robot_habilitado = false;
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;
  mandar_robot_a_ataca(); 
  server.send(200, "text/plain", "Robot parado; listo en estado ATACA");
}

void handleReset() {
  resetear_MEF();
  server.send(200, "text/plain", "MEF reseteada a estado ATACA");
}

void handleSaveConfig() {
  guardar_configuracion();
  server.send(200, "text/plain", "Configuracion guardada en memoria NVS");
}

void handleDefaultConfig() {
  restaurar_configuracion_por_defecto();
  server.send(200, "text/plain", "Valores por defecto restaurados y guardados en memoria");
}

void handleNotFound() {
  server.send(404, "text/plain", "Ruta no encontrada");
}

uint32_t leerArgU32(const char* nombre, uint32_t actual, uint32_t minimo, uint32_t maximo) {
  if (!server.hasArg(nombre)) return actual;
  long valor = server.arg(nombre).toInt();
  if (valor < (long)minimo) return minimo;
  if (valor > (long)maximo) return maximo;
  return (uint32_t)valor;
}

int16_t leerArgI16(const char* nombre, int16_t actual, int16_t minimo, int16_t maximo) {
  if (!server.hasArg(nombre)) return actual;
  long valor = server.arg(nombre).toInt();
  if (valor < minimo) return minimo;
  if (valor > maximo) return maximo;
  return (int16_t)valor;
}

float leerArgFloat(const char* nombre, float actual, float minimo, float maximo) {
  if (!server.hasArg(nombre)) return actual;
  float valor = server.arg(nombre).toFloat();
  if (valor < minimo) return minimo;
  if (valor > maximo) return maximo;
  return valor;
}

// ============================================================
// JSON STATUS
// ============================================================
void enviarJSONStatus() {
  String json;
  json.reserve(1700);

  json += "{";
  json += "\"robot_habilitado\":";
  json += robot_habilitado ? "true" : "false";
  json += ",";
  json += "\"estado\":\"";
  json += estadoToTexto(estado_actual);
  json += "\",";
  json += "\"laser_mm\":";
  json += String(laser);
  json += ",";
  json += "\"laser_ataque_mm\":";
  if (distancia_inicio_ataque_mm < 0) {
    json += "null";
  } else {
    json += String(distancia_inicio_ataque_mm);
  }
  json += ",";
  json += "\"laser_detectado\":";
  json += laser_detectado ? "true" : "false";
  json += ",";
  json += "\"consigna_dcha\":";
  json += String(CONSIGNA_DCHA);
  json += ",";
  json += "\"consigna_izda\":";
  json += String(CONSIGNA_IZDA);
  json += ",";
  json += "\"tl\":";
  json += String(TICKER_LASER_MS);
  json += ",";
  json += "\"tm\":";
  json += String(TICKER_MOTORES_MS);
  json += ",";
  json += "\"tf\":";
  json += String(TICKER_MEF_MS);
  json += ",";
  json += "\"kp\":";
  json += String(getKpControl(), 3);
  json += ",";
  json += "\"ki\":";
  json += String(getKiControl(), 3);
  json += ",";
  json += "\"vmax\":";
  json += String(getVmaxControl(), 3);
  json += ",";
  json += "\"umbral\":";
  json += String(dist_activacion);
  json += ",";
  json += "\"reposo\":";
  json += String(T_REPOSO);
  json += ",";
  json += "\"t_tran\":";
  json += String(T_TRAN);
  json += ",";
  json += "\"ataque\":";
  json += String(T_ATAQUE);
  json += ",";
  json += "\"retroceso\":";
  json += String(T_RETROCESO);
  json += ",";
  json += "\"fin_busqueda\":";
  json += String(T_fin_busqueda);

  json += ",";
  json += "\"v_b1_d\":";
  json += String(V_BUSCA_1_DCHA);
  json += ",";
  json += "\"v_b1_i\":";
  json += String(V_BUSCA_1_IZDA);
  json += ",";
  json += "\"v_b2_d\":";
  json += String(V_BUSCA_2_DCHA);
  json += ",";
  json += "\"v_b2_i\":";
  json += String(V_BUSCA_2_IZDA);
  json += ",";
  json += "\"v_b3_d\":";
  json += String(V_BUSCA_3_DCHA);
  json += ",";
  json += "\"v_b3_i\":";
  json += String(V_BUSCA_3_IZDA);
  json += ",";
  json += "\"v_ataca_d\":";
  json += String(V_ATACA_DCHA);
  json += ",";
  json += "\"v_ataca_i\":";
  json += String(V_ATACA_IZDA);
  json += ",";
  json += "\"v_retro_d\":";
  json += String(V_RETROCEDE_DCHA);
  json += ",";
  json += "\"v_retro_i\":";
  json += String(V_RETROCEDE_IZDA);
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ============================================================
// TEXTO ESTADOS
// ============================================================
const char* estadoToTexto(Estado_t estado) {
  switch (estado) {
    case REPOSO:    return "REPOSO";
    case BUSCA_1:   return "BUSCA_1";
    case BUSCA_2:   return "BUSCA_2";
    case BUSCA_3:   return "BUSCA_3";
    case TRANSIATACA: return "TRANSIATACA";
    case ATACA:     return "ATACA";
    case RETROCEDE: return "RETROCEDE";
    case E6:        return "E6";
    case E7:        return "E7";
    case E8:        return "E8";
    default:        return "DESCONOCIDO";
  }
}