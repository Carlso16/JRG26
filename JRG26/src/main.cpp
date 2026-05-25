#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>

#include "Laser_2.h"
#include "Servos.h"

// ============================================================
// WIFI / SERVIDOR WEB
// ============================================================

const char* WIFI_SSID = "SUMO_ROBOT";
const char* WIFI_PASS = "12345678";   // minimo 8 caracteres para SoftAP

WebServer server(80);

// ============================================================
// VELOCIDADES DEL ROBOT
// ============================================================
// IMPORTANTE:
// Antes estaban como #define. Asi no se pueden cambiar desde la web.
// Ahora son variables para poder modificarlas en ejecucion.
//
// Rango recomendado: -100 a 100, aunque tu control actual trabaja bien
// con valores pequenos tipo +-15, +-20, +-25.

volatile int16_t V_INICIO_DCHA    = -20;
volatile int16_t V_INICIO_IZDA    = -20;

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
// No se toca la libreria de motores. Se mantiene tu llamada original.
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
uint32_t T_inicio    = 800;

// ============================================================
// PERIODOS TICKER
// ============================================================
// Se permite cambiar laser y MEF desde la web.
// El ticker de motores se deja fijo porque funciona bien y no queremos tocarlo.

uint32_t TICKER_LASER_MS   = 50;
const uint32_t TICKER_MOTORES_MS = 10;
uint32_t TICKER_MEF_MS     = 100;

// ============================================================
// LASER
// ============================================================

#define DIR_LASER 0x29

volatile int16_t laser = -1;
int16_t dist_activacion = 800;
volatile bool laser_detectado = false;

// Distancia del laser que provoco el ultimo cambio BUSCA_3 -> ATACA.
// Valor -1 significa None: todavia no se ha producido ese cambio en el ciclo actual.
volatile int16_t distancia_inicio_ataque_mm = -1;

// ============================================================
// CONTROL GENERAL
// ============================================================

volatile bool robot_habilitado = true;

// ============================================================
// FLAGS DE TAREAS
// IMPORTANTE:
// El Ticker del laser NO lee I2C. Solo activa una bandera.
// La lectura real del VL53L1X se hace en loop().
// ============================================================

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
  INICIO,
  REPOSO,
  BUSCA_1,
  BUSCA_2,
  BUSCA_3,
  ATACA,
  RETROCEDE,
  E6,
  E7,
  E8
} Estado_t;

volatile Estado_t estado_actual = INICIO;

// ============================================================
// PROTOTIPOS
// ============================================================

void initSUMO();
void init_wifi_ap();
void init_servidor_web();

void reprogramar_ticker_laser();
void reprogramar_ticker_mef();

void cb_ticker_laser();
void atender_laser_pendiente();

int16_t leer_laser();
void actualizar_laser();
void limpiar_distancia_inicio_ataque();
void registrar_distancia_inicio_ataque();

void mover_robot(float velocidadDcha, float velocidadIzda);
void aplicar_MEF();
void aplicar_motores();

void parar_robot_logico();
void mandar_robot_a_reposo();
void aplicar_estado_start_stop();
void resetear_MEF();

const char* estadoToTexto(Estado_t estado);

String paginaHTML();
void handleRoot();
void handleStatus();
void handleConfig();
void handleStart();
void handleStop();
void handleReset();
void handleNotFound();

uint32_t leerArgU32(const char* nombre, uint32_t actual, uint32_t minimo, uint32_t maximo);
int16_t leerArgI16(const char* nombre, int16_t actual, int16_t minimo, int16_t maximo);
float leerArgFloat(const char* nombre, float actual, float minimo, float maximo);
void enviarJSONStatus();

// ============================================================
// SETUP
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

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  server.handleClient();
  atender_laser_pendiente();
}

// ============================================================
// INIT GENERAL
// ============================================================

void initSUMO() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

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

  // Laser corregido: el callback del ticker ya no toca I2C.
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
  server.onNotFound(handleNotFound);
  server.begin();
}

// ============================================================
// TICKERS
// ============================================================

void reprogramar_ticker_laser() {
  ticker_laser.detach();

  // El VL53L1X esta configurado con timing budget de 50 ms.
  // Bajar de 50 ms no aporta y puede generar lecturas repetidas o inestables.
  TICKER_LASER_MS = constrain(TICKER_LASER_MS, 50UL, 1000UL);

  ticker_laser.attach_ms(TICKER_LASER_MS, cb_ticker_laser);
}

void reprogramar_ticker_mef() {
  ticker_mef.detach();

  // No lo bajes demasiado: la MEF no necesita ir al ritmo del control de motor.
  TICKER_MEF_MS = constrain(TICKER_MEF_MS, 10UL, 1000UL);

  ticker_mef.attach_ms(TICKER_MEF_MS, aplicar_MEF);
}

void cb_ticker_laser() {
  flag_laser = true;
}

void atender_laser_pendiente() {
  if (!flag_laser) {
    return;
  }

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
    case INICIO:
      // Movimiento inicial.
      if (t0 == 0) {
        t0 = millis();
      }

      CONSIGNA_DCHA = V_INICIO_DCHA;
      CONSIGNA_IZDA = V_INICIO_IZDA;

      if (millis() - t0 >= T_inicio) {
        estado_actual = REPOSO;
        t0 = millis();
      }
      break;

    case REPOSO:
      // Reposo SOLO tras el inicio/reset.
      // Despues de RETROCEDE se vuelve directamente a BUSCA_1.
      CONSIGNA_DCHA = 0;
      CONSIGNA_IZDA = 0;

      if (millis() - t0 >= T_REPOSO) {
        limpiar_distancia_inicio_ataque();
        estado_actual = BUSCA_1;
        t0 = millis();
      }
      break;

    case BUSCA_1:
      // BUSQUEDA HASTA VER
      if (laser_detectado) {
        estado_actual = BUSCA_2;
        t0 = millis();
      }

      CONSIGNA_DCHA = V_BUSCA_1_DCHA;
      CONSIGNA_IZDA = V_BUSCA_1_IZDA;
      break;

    case BUSCA_2:
      // BUSQUEDA HASTA NO VER
      if (!laser_detectado) {
        estado_actual = BUSCA_3;
        t0 = millis();
      }

      CONSIGNA_DCHA = V_BUSCA_2_DCHA;
      CONSIGNA_IZDA = V_BUSCA_2_IZDA;
      break;

    case BUSCA_3:
      // BUSQUEDA HASTA VOLVER A VER
      if (laser_detectado) {
        registrar_distancia_inicio_ataque();
        estado_actual = ATACA;
        t0 = millis();
      }

      CONSIGNA_DCHA = V_BUSCA_3_DCHA;
      CONSIGNA_IZDA = V_BUSCA_3_IZDA;
      break;

    case ATACA:
      if (millis() - t0 >= T_ATAQUE) {
        estado_actual = RETROCEDE;
        t0 = millis();
      }

      CONSIGNA_DCHA = V_ATACA_DCHA;
      CONSIGNA_IZDA = V_ATACA_IZDA;
      break;

    case RETROCEDE:
      if (millis() - t0 >= T_RETROCESO) {
        // CRITICO:
        // Aqui NO se pasa por REPOSO.
        // El algoritmo queda en bucle:
        // BUSCA_1 -> BUSCA_2 -> BUSCA_3 -> ATACA -> RETROCEDE -> BUSCA_1
        limpiar_distancia_inicio_ataque();
        estado_actual = BUSCA_1;
        t_atacado = 0;
        t0 = millis();
      }

      CONSIGNA_DCHA = V_RETROCEDE_DCHA;
      CONSIGNA_IZDA = V_RETROCEDE_IZDA;
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
// LASER
// ============================================================

void actualizar_laser() {
  // Lectura real por I2C fuera del callback del Ticker.
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
  // No leemos I2C aqui porque aplicar_MEF() viene de un Ticker.
  // Guardamos la ultima lectura valida ya tomada en loop().
  distancia_inicio_ataque_mm = -1;

  if (laser >= 0) {
    distancia_inicio_ataque_mm = laser;
  }
}

// ============================================================
// MOTORES
// ============================================================

void aplicar_motores() {
  mover_robot(CONSIGNA_DCHA, CONSIGNA_IZDA);
}

void mover_robot(float velocidadDcha, float velocidadIzda) {
  // Se mantiene tu estructura original:
  // derecha se manda a DD y TD, izquierda se manda al lado izquierdo.
  wRuedas(velocidadDcha, velocidadDcha, velocidadIzda);
}

void parar_robot_logico() {
  robot_habilitado = false;
  mandar_robot_a_reposo();
}

void mandar_robot_a_reposo() {
  estado_actual = REPOSO;
  t0 = millis();
  t_atacado = 0;
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;
}

void aplicar_estado_start_stop() {
  // START y STOP no deben reiniciar ni modificar INICIO.
  // Si el robot esta en INICIO, se conserva ese estado.
  // Si esta en cualquier otro estado, se manda a REPOSO.
  if (estado_actual == INICIO) {
    return;
  }

  mandar_robot_a_reposo();
}

void resetear_MEF() {
  limpiar_distancia_inicio_ataque();
  estado_actual = INICIO;
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
  html += F("<p><small>Al guardar Kp/Ki/Vmax se resetea la integral del PI para evitar arrastre de saturacion.</small></p>");

  html += F("<h4>Laser</h4>");
  html += F("<label>Umbral deteccion laser [mm]</label><input id='umbral' type='number' min='20' max='4000'><br>");

  html += F("<h4>Tiempos MEF [ms]</h4>");
  html += F("<label>T_inicio</label><input id='inicio' type='number' min='0' max='20000'><br>");
  html += F("<label>T_REPOSO solo tras inicio/reset</label><input id='reposo' type='number' min='0' max='20000'><br>");
  html += F("<label>T_ATAQUE</label><input id='ataque' type='number' min='0' max='20000'><br>");
  html += F("<label>T_RETROCESO</label><input id='retroceso' type='number' min='0' max='20000'><br>");

  html += F("<h4>Velocidades [-100 a 100]</h4>");
  html += F("<div class='fila'><label>INICIO DCHA / IZDA</label><input id='v_inicio_d' type='number' min='-100' max='100'> <input id='v_inicio_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>BUSCA_1 DCHA / IZDA</label><input id='v_b1_d' type='number' min='-100' max='100'> <input id='v_b1_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>BUSCA_2 DCHA / IZDA</label><input id='v_b2_d' type='number' min='-100' max='100'> <input id='v_b2_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>BUSCA_3 DCHA / IZDA</label><input id='v_b3_d' type='number' min='-100' max='100'> <input id='v_b3_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>ATACA DCHA / IZDA</label><input id='v_ataca_d' type='number' min='-100' max='100'> <input id='v_ataca_i' type='number' min='-100' max='100'></div>");
  html += F("<div class='fila'><label>RETROCEDE DCHA / IZDA</label><input id='v_retro_d' type='number' min='-100' max='100'> <input id='v_retro_i' type='number' min='-100' max='100'></div>");

  html += F("<br><button onclick='guardar()'>Guardar configuracion</button>");
  html += F("<p id='msg'></p>");
  html += F("</div>");

  html += F("<script>");
  html += F("let primera=true;");
  html += F("const campos=['tl','tf','kp','ki','vmax','umbral','inicio','reposo','ataque','retroceso','v_inicio_d','v_inicio_i','v_b1_d','v_b1_i','v_b2_d','v_b2_i','v_b3_d','v_b3_i','v_ataca_d','v_ataca_i','v_retro_d','v_retro_i'];");
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
  html += F("async function guardar(){let p=new URLSearchParams();campos.forEach(k=>p.append(k,document.getElementById(k).value));let r=await fetch('/api/config?'+p.toString());document.getElementById('msg').innerText=await r.text();primera=true;estado();}");
  html += F("async function startRobot(){await fetch('/api/start');estado();}");
  html += F("async function stopRobot(){await fetch('/api/stop');estado();}");
  html += F("async function resetRobot(){await fetch('/api/reset');estado();}");
  html += F("setInterval(estado,500);estado();");
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

  float kp = leerArgFloat("kp", getKpControl(), 0.0f, 5.0f);
  float ki = leerArgFloat("ki", getKiControl(), 0.0f, 100.0f);
  float vmax = leerArgFloat("vmax", getVmaxControl(), 1.0f, 20.0f);
  setConstantesControl(kp, ki, vmax, true);

  dist_activacion = leerArgI16("umbral", dist_activacion, 20, 4000);

  T_inicio    = leerArgU32("inicio", T_inicio, 0, 20000);
  T_REPOSO    = leerArgU32("reposo", T_REPOSO, 0, 20000);
  T_ATAQUE    = leerArgU32("ataque", T_ATAQUE, 0, 20000);
  T_RETROCESO = leerArgU32("retroceso", T_RETROCESO, 0, 20000);

  V_INICIO_DCHA    = leerArgI16("v_inicio_d", V_INICIO_DCHA, -100, 100);
  V_INICIO_IZDA    = leerArgI16("v_inicio_i", V_INICIO_IZDA, -100, 100);

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

  server.send(200, "text/plain", "Configuracion guardada");
}

void handleStart() {
  bool estaba_en_inicio = (estado_actual == INICIO);

  robot_habilitado = true;
  aplicar_estado_start_stop();

  if (estaba_en_inicio) {
    server.send(200, "text/plain", "Robot habilitado; estado INICIO conservado");
  } else {
    server.send(200, "text/plain", "Robot habilitado; enviado a REPOSO");
  }
}

void handleStop() {
  bool estaba_en_inicio = (estado_actual == INICIO);

  robot_habilitado = false;
  aplicar_estado_start_stop();

  // Parada inmediata de consigna sin cambiar la MEF si estaba en INICIO.
  CONSIGNA_DCHA = 0;
  CONSIGNA_IZDA = 0;

  if (estaba_en_inicio) {
    server.send(200, "text/plain", "Robot parado; estado INICIO conservado");
  } else {
    server.send(200, "text/plain", "Robot parado; enviado a REPOSO");
  }
}

void handleReset() {
  resetear_MEF();
  server.send(200, "text/plain", "MEF reseteada");
}

void handleNotFound() {
  server.send(404, "text/plain", "Ruta no encontrada");
}

uint32_t leerArgU32(const char* nombre, uint32_t actual, uint32_t minimo, uint32_t maximo) {
  if (!server.hasArg(nombre)) {
    return actual;
  }

  long valor = server.arg(nombre).toInt();

  if (valor < (long)minimo) {
    return minimo;
  }

  if (valor > (long)maximo) {
    return maximo;
  }

  return (uint32_t)valor;
}

int16_t leerArgI16(const char* nombre, int16_t actual, int16_t minimo, int16_t maximo) {
  if (!server.hasArg(nombre)) {
    return actual;
  }

  long valor = server.arg(nombre).toInt();

  if (valor < minimo) {
    return minimo;
  }

  if (valor > maximo) {
    return maximo;
  }

  return (int16_t)valor;
}

float leerArgFloat(const char* nombre, float actual, float minimo, float maximo) {
  if (!server.hasArg(nombre)) {
    return actual;
  }

  float valor = server.arg(nombre).toFloat();

  if (valor < minimo) {
    return minimo;
  }

  if (valor > maximo) {
    return maximo;
  }

  return valor;
}

void enviarJSONStatus() {
  String json;
  json.reserve(1800);

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
  json += "\"inicio\":";
  json += String(T_inicio);
  json += ",";
  json += "\"reposo\":";
  json += String(T_REPOSO);
  json += ",";
  json += "\"ataque\":";
  json += String(T_ATAQUE);
  json += ",";
  json += "\"retroceso\":";
  json += String(T_RETROCESO);

  json += ",";
  json += "\"v_inicio_d\":";
  json += String(V_INICIO_DCHA);
  json += ",";
  json += "\"v_inicio_i\":";
  json += String(V_INICIO_IZDA);

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
    case INICIO:    return "INICIO";
    case REPOSO:    return "REPOSO";
    case BUSCA_1:   return "BUSCA_1";
    case BUSCA_2:   return "BUSCA_2";
    case BUSCA_3:   return "BUSCA_3";
    case ATACA:     return "ATACA";
    case RETROCEDE: return "RETROCEDE";
    case E6:        return "E6";
    case E7:        return "E7";
    case E8:        return "E8";
    default:        return "DESCONOCIDO";
  }
}
