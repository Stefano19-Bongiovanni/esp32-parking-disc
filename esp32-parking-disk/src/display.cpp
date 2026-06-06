#include "display.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Prima scelta per molti 2.13" BW 122x250 con controller SSD1680
GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>
    display(GxEPD2_213_GDEY0213B74(PIN_CS, PIN_DC, PIN_RES, PIN_BUSY));

// Se questa variante non funziona sul tuo pannello, prova a sostituire
// SOLO le due righe sopra con queste:
//
// GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT>
// display(GxEPD2_213_BN(PIN_CS, PIN_DC, PIN_RES, PIN_BUSY));

void initDisplay()
{
    Serial.println("display: init...");

    // SPI esplicita sui pin reali dell'ESP32-C3 SuperMini
    SPI.begin(PIN_SCL, -1, PIN_SDA, PIN_CS);

    // init serial debug della libreria; i parametri extra sono usati spesso con WeAct
    display.init(115200, true, 50, false);

    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 30);
        display.println("starting...");

        display.setCursor(10, 55);
        display.println("ESP32-C3");

        display.setCursor(10, 80);
        display.println("WeAct 2.13 BW");
    } while (display.nextPage());

    display.hibernate();

    Serial.println("display: done");
}