#include "display.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <OswaldRegular40pt7b.h>
#include <math.h>

// Alias unico — cambia solo qui quando switching font
static const GFXfont *const CLOCK_FONT = &Oswald_Regular40pt7b;

// Prima scelta per molti 2.13" BW 122x250 con controller SSD1680
GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT>
    display(GxEPD2_213_GDEY0213B74(PIN_CS, PIN_DC, PIN_RES, PIN_BUSY));

// Se questa variante non funziona sul tuo pannello, prova a sostituire
// SOLO le due righe sopra con queste:
//
// GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT>
// display(GxEPD2_213_BN(PIN_CS, PIN_DC, PIN_RES, PIN_BUSY));

// ---------------------------------------------------------------------------
// Costanti geometriche — specchio del canvas HTML (250×122)
// ---------------------------------------------------------------------------

// Raggio del cerchio virtuale (identico a R nel JS)
static constexpr float R = 400.0f;

// Offset verticale origine (identico a topOffset nel JS)
static constexpr float TOP_OFFSET = 16.0f;

// Lo step angolare tra un'ora e la successiva su 24 divisioni
static constexpr float ANGLE_SPACE = 360.0f / 24.0f; // 15°

// Origine del cerchio: centro X del canvas, molto in alto rispetto al canvas
// origin.y = canvas.height/2 - R + topOffset  →  122/2 - 400 + 16 = -323
static constexpr float ORIGIN_X = 250.0f / 2.0f;
static constexpr float ORIGIN_Y = 122.0f / 2.0f - R + TOP_OFFSET;

// Dimensioni tick: { larghezza, altezza } in pixel
struct TickSize
{
    int w;
    int h;
};
static constexpr TickSize TICK_LARGE = {4, 20};
static constexpr TickSize TICK_MEDIUM = {2, 14};
static constexpr TickSize TICK_SMALL = {2, 6};

// Tabella dei 15 tick visibili (angoli relativi allo zero, in gradi)
struct TickDef
{
    const char *size;
    float angle;
};
static const TickDef TICKS[] = {
    {"small", -(7.0f * ANGLE_SPACE) / 4.0f},
    {"medium", -(6.0f * ANGLE_SPACE) / 4.0f},
    {"small", -(5.0f * ANGLE_SPACE) / 4.0f},
    {"large", -(4.0f * ANGLE_SPACE) / 4.0f},
    {"small", -(3.0f * ANGLE_SPACE) / 4.0f},
    {"medium", -(2.0f * ANGLE_SPACE) / 4.0f},
    {"small", -(1.0f * ANGLE_SPACE) / 4.0f},
    {"large", 0.0f},
    {"small", (1.0f * ANGLE_SPACE) / 4.0f},
    {"medium", (2.0f * ANGLE_SPACE) / 4.0f},
    {"small", (3.0f * ANGLE_SPACE) / 4.0f},
    {"large", (4.0f * ANGLE_SPACE) / 4.0f},
    {"small", (5.0f * ANGLE_SPACE) / 4.0f},
    {"medium", (6.0f * ANGLE_SPACE) / 4.0f},
    {"small", (7.0f * ANGLE_SPACE) / 4.0f},
};
static constexpr int NUM_TICKS = sizeof(TICKS) / sizeof(TICKS[0]);

// ---------------------------------------------------------------------------
// Helpers geometrici
// ---------------------------------------------------------------------------

static inline float degToRad(float deg)
{
    return deg * (float)M_PI / 180.0f;
}

// Converte un angolo (gradi) in coordinate canvas, con offset radiale opzionale
static void angleToXY(float angleDeg, float radiusOffset, float &x, float &y)
{
    float rad = degToRad(angleDeg);
    x = ORIGIN_X + (R + radiusOffset) * cosf(rad);
    y = ORIGIN_Y + (R + radiusOffset) * sinf(rad);
}

// Wrap ciclico 0–23
static inline int wrap24(int n)
{
    return ((n % 24) + 24) % 24;
}

// ---------------------------------------------------------------------------
// Funzioni di disegno
// ---------------------------------------------------------------------------

// Disegna un singolo cifra (0–23) ruotata attorno all'arco, all'angolo dato.
// Usa FreeMonoBold9pt7b (altezza ~14px) scalato ×6 via GFX setTextSize
// per avvicinarsi al font-size 80 dell'HTML (9pt × ~6 ≈ 78px).
static void drawAngledNumber(int number, float angleDeg)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", number);

    display.setTextSize(1);
    display.setFont(CLOCK_FONT);

    // 1. Misura dimensioni
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(buf, 0, 0, &tbx, &tby, &tbw, &tbh);

    // 2. Alloca buffer temporaneo (1 bit per pixel, arrotondato al byte)
    uint16_t bufW = tbw + 4;
    uint16_t bufH = tbh + 4;
    size_t bufSize = ((bufW + 7) / 8) * bufH;
    uint8_t *bmp = (uint8_t *)calloc(bufSize, 1);
    if (!bmp)
        return;

    // Riempi tutto di bianco (1)
    memset(bmp, 0xFF, bufSize);

    // 3. Usa GFXcanvas1 per disegnare il testo offline
    GFXcanvas1 canvas(bufW, bufH);
    canvas.fillScreen(GxEPD_WHITE); // sfondo bianco
    canvas.setFont(CLOCK_FONT);
    canvas.setTextColor(GxEPD_BLACK);
    canvas.setCursor(-tbx + 2, -tby + 2); // offset per bounding box
    canvas.print(buf);

    // 4. Calcola posizione centro sul display
    float cx, cy;
    angleToXY(angleDeg, 0.0f, cx, cy);

    // 5. Angolo di rotazione: tangente al cerchio
    float rad = degToRad(angleDeg) - (float)M_PI / 2.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    // 6. Copia pixel con rotazione
    for (int py = 0; py < bufH; py++)
    {
        for (int px = 0; px < bufW; px++)
        {
            // Offset dal centro della bitmap
            float dx = px - bufW / 2.0f;
            float dy = py - bufH / 2.0f;

            // Ruota
            int sx = (int)roundf(cx + dx * cosA - dy * sinA);
            int sy = (int)roundf(cy + dx * sinA + dy * cosA);

            // Disegna solo pixel neri
            if (canvas.getPixel(px, py) == GxEPD_BLACK)
            {
                if (sx >= 0 && sx < 250 && sy >= 0 && sy < 122)
                    display.drawPixel(sx, sy, GxEPD_BLACK);
            }
        }
    }

    free(bmp); // GFXcanvas1 gestisce la sua memoria internamente
}

// Disegna un tick mark ruotato all'angolo dato
static void drawTick(const TickDef &tick, float shiftDeg)
{
    const TickSize *ts;
    if (strcmp(tick.size, "large") == 0)
        ts = &TICK_LARGE;
    else if (strcmp(tick.size, "medium") == 0)
        ts = &TICK_MEDIUM;
    else
        ts = &TICK_SMALL;

    // Offset radiale: i tick sono a -65 + h/2 dal bordo del cerchio (come nel JS)
    float radiusOffset = -65.0f + ts->h / 2.0f;
    float angleDeg = 90.0f + tick.angle + shiftDeg;

    float cx, cy;
    angleToXY(angleDeg, radiusOffset, cx, cy);

    // Ruota il rettangolo del tick di (angleDeg + 90°) attorno al suo centro.
    // Implementazione: calcola i 4 angoli del rettangolo, ruotali, riempili
    // con linee verticali (pixel-fill manuale compatibile con GxEPD2).
    float rot = degToRad(angleDeg + 90.0f);
    float cosR = cosf(rot);
    float sinR = sinf(rot);

    int hw = ts->w / 2;
    int hh = ts->h / 2;

    // I 4 vertici del rettangolo centrato in (0,0) prima della rotazione
    float lx[4] = {(float)-hw, (float)hw, (float)hw, (float)-hw};
    float ly[4] = {(float)-hh, (float)-hh, (float)hh, (float)hh};

    // Ruota e trasla
    int px[4], py[4];
    for (int i = 0; i < 4; i++)
    {
        px[i] = (int)roundf(cx + lx[i] * cosR - ly[i] * sinR);
        py[i] = (int)roundf(cy + lx[i] * sinR + ly[i] * cosR);
    }

    // Fill del rettangolo ruotato via scanline sull'asse locale
    // Usiamo drawLine tra i lati lunghi, campionando lungo l'asse del tick
    int steps = ts->h;
    for (int s = 0; s <= steps; s++)
    {
        float t = (float)s / (float)steps - 0.5f; // -0.5 .. +0.5
        // Punto sul lato sinistro e destro lungo l'asse lungo
        int x1 = (int)roundf(cx + (-hw) * cosR - (t * ts->h) * sinR);
        int y1 = (int)roundf(cy + (-hw) * sinR + (t * ts->h) * cosR);
        int x2 = (int)roundf(cx + (hw)*cosR - (t * ts->h) * sinR);
        int y2 = (int)roundf(cy + (hw)*sinR + (t * ts->h) * cosR);
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
    }
}

// ---------------------------------------------------------------------------
// API pubblica
// ---------------------------------------------------------------------------

void initDisplay()
{
    Serial.println("display: init...");

    SPI.begin(PIN_SCL, -1, PIN_SDA, PIN_CS);

    display.init(115200, true, 50, false);

    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(CLOCK_FONT);

    Serial.println("display: ready");
}

void showInitialScreen()
{
    display.setFullWindow();

    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        const char *msg = "UwU...";

        int16_t tbx, tby;
        uint16_t tbw, tbh;
        display.getTextBounds(msg, 0, 0, &tbx, &tby, &tbw, &tbh);

        int16_t x = (display.width() - tbw) / 2 - tbx;
        int16_t y = (display.height() - tbh) / 2 - tby;

        display.setCursor(x, y);
        display.print(msg);

    } while (display.nextPage());

    display.hibernate();
}

// Renderizza un'immagine bitmap 1bpp ricevuta via BLE.
// Tight-packed, MSB-first: bit=1 -> pixel NERO, bit=0 -> pixel BIANCO.
// Pixel index: i = y * IMAGE_WIDTH + x  →  byte i>>3, bit 7-(i&7).
void drawImage(const uint8_t *data, size_t len)
{
    static constexpr uint16_t IMAGE_WIDTH = 250;
    static constexpr uint16_t IMAGE_HEIGHT = 122;
    static constexpr size_t IMAGE_BYTES = ((uint32_t)IMAGE_WIDTH * IMAGE_HEIGHT + 7) / 8; // 3813

    if (!data || len < IMAGE_BYTES)
        return;

    initDisplay();

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        for (uint16_t y = 0; y < IMAGE_HEIGHT; y++)
        {
            for (uint16_t x = 0; x < IMAGE_WIDTH; x++)
            {
                uint32_t idx = (uint32_t)y * IMAGE_WIDTH + x;
                uint8_t byte_val = data[idx >> 3];
                uint8_t bit = (byte_val >> (7 - (idx & 7))) & 0x01;
                if (bit)
                    display.drawPixel(x, y, GxEPD_BLACK);
            }
        }

    } while (display.nextPage());

    display.hibernate();
}

// Replica esatta di updateNumber() dal canvas HTML.
// number: valore float 0.0–23.75 (step tipico 0.25)
void updateNumber(float number)
{
    initDisplay(); // re-init per uscire da deep sleep, se necessario

    // Arrotonda al valore intero più vicino e calcola la frazione residua
    int value = (int)roundf(number);
    float decimal = number - (float)value;
    float shift = decimal * ANGLE_SPACE; // shift in gradi

    float baseAngle = 90.0f + shift;

    display.setFullWindow();
    display.firstPage();
    do
    {
        // Sfondo bianco
        display.fillScreen(GxEPD_WHITE);

        // Numero centrale (con wrap 0–23)
        drawAngledNumber(wrap24(value), baseAngle);

        // Numero precedente (a sinistra sull'arco = angolo maggiore)
        drawAngledNumber(wrap24(value - 1), baseAngle + ANGLE_SPACE);

        // Numero successivo (a destra sull'arco = angolo minore)
        drawAngledNumber(wrap24(value + 1), baseAngle - ANGLE_SPACE);

        // Tick marks
        for (int i = 0; i < NUM_TICKS; i++)
        {
            drawTick(TICKS[i], shift);
        }

    } while (display.nextPage());

    display.hibernate();
}