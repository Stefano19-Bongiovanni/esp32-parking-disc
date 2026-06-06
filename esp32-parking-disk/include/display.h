#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Connessioni fisiche (note: VCC e GND sono alimentazione, non GPIO)
// VCC -> 3V3
// GND -> GND

// Pin definiti per collegare il WeAct 2.13" Epaper Module
constexpr int PIN_VCC = -1;     // collegato a 3V3 (non GPIO)
constexpr int PIN_GND = -1;     // collegato a GND (non GPIO)
constexpr uint8_t PIN_SCL = 4;  // SCL / SCK
constexpr uint8_t PIN_SDA = 6;  // SDA / MOSI
constexpr uint8_t PIN_CS = 7;   // CS (chip select)
constexpr uint8_t PIN_DC = 1;   // D/C (data/command)
constexpr uint8_t PIN_RES = 2;  // RES / RST
constexpr uint8_t PIN_BUSY = 3; // BUSY

// Inizializza il display e mostra un messaggio iniziale "starting..."
void initDisplay();

#endif // DISPLAY_H
