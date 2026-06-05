#include <Arduino.h>
#include "display.h"

// put function declarations here:
int myFunction(int, int);

void setup() {
  // inizializza seriale per debug
  Serial.begin(115200);
  delay(5);

  // inizializza il display (assume Serial già inizializzato)
  initDisplay();

  // esempio: uso della funzione esistente
  int result = myFunction(2, 3);
  (void)result;
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}