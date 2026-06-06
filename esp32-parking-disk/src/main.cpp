#include <Arduino.h>
#include "display.h"

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("setup: avvio del sistema...");
  initDisplay();
  Serial.println("setup: sistema avviato, inizio loop...");
}

void loop()
{
  static float current = 0.0f;

  Serial.print("updateNumber: ");
  Serial.println(current, 2);

  updateNumber(current);

  current += 0.25f;
  if (current >= 24.0f)
    current = 0.0f;

  delay(2000);
}