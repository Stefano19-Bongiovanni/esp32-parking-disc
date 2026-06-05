#include "display.h"
#include <Arduino.h>
#include <SPI.h>

void initDisplay()
{
    // Configuro i pin di controllo
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RES, OUTPUT);
    pinMode(PIN_BUSY, INPUT);

    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_RES, HIGH);

    // Avvia SPI (usa i pin hardware di default dell'ESP32-C3)
    SPI.begin();

    // Reset hardware del display
    digitalWrite(PIN_RES, LOW);
    delay(10);
    digitalWrite(PIN_RES, HIGH);
    delay(10);

    // Attende che il BUSY si liberi (timeout 2s)
    unsigned long start = millis();
    while (digitalRead(PIN_BUSY) == HIGH && (millis() - start) < 2000) {
        delay(10);
    }

    // Nota: per rendere testo reale sul WeAct 2.13" serve una libreria grafica
    // (es. GxEPD2 + Adafruit_GFX). Qui inviamo un semplice blocco dati via SPI
    // come prova; per il rendering corretto integrare la libreria desiderata.

    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_CS, LOW);

    // Invio un comando dummy (DC LOW) seguito da dati (DC HIGH)
    digitalWrite(PIN_DC, LOW);
    SPI.transfer((uint8_t)0x00); // comando di test (arbitrario)

    digitalWrite(PIN_DC, HIGH);
    const char* msg = "starting...";
    for (const char* p = msg; *p; ++p) {
        SPI.transfer((uint8_t)(*p));
    }

    digitalWrite(PIN_CS, HIGH);
    SPI.endTransaction();

    // Stampare su Serial presuppone che Serial sia inizializzata dal chiamante
    Serial.println("display: messaggio inviato (per visualizzare testo usare una libreria e-paper)");
}
