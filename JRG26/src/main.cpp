#include <Arduino.h>
#include "Servos.h" 
#include <Ticker.h>

Ticker tickerControl;

int spTD = 0;
int spI  = 0;

void IRAM_ATTR controlLoop() { wRuedas(0, spTD, spI); }
void setup() {
  Serial.begin(115200);
  initPWM(); 
  
  tickerControl.attach_ms(10, controlLoop);
}

void loop() {
spTD = 0;
spI = 0;
delay(2000);

spTD = 20;
spI = 20;
delay(1269);

spTD = 20;
spI = -20;
delay(600);

spTD = 20;
spI = 20;
delay(1269);

spTD = 20;
spI = -20;
delay(600);

spTD = 20;
spI = 20;
delay(1269);

spTD = 20;
spI = -20;
delay(600);

spTD = 20;
spI = 20;
delay(1269);

spTD = 0;
spI = 0;

delay(20000);

}