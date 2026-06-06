#include <Arduino.h>
#include "display.h"

void setup()
{
  // USB CDC Serial (quello che vedi sul PC)
  Serial.begin(115200);
  delay(1000); // importante! dai tempo all'host di aprire la porta

  Serial.println("setup: avvio del sistema...");
  initDisplay();
  Serial.println("setup: sistema avviato, inizio loop...");
}

void loop()
{
  Serial.println("looping...");
  delay(2000);
}