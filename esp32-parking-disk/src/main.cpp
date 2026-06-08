// main.cpp
#include <Arduino.h>
#include <esp_sleep.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "display.h"
#include "power.h"
#include <esp_wifi.h>

#define BLE_SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define BLE_CHARACTERISTIC_UUID "12345678-1234-1234-1234-123456789def"

// Dimensioni immagine 1bpp trasmessa via BLE
static constexpr uint16_t IMAGE_WIDTH = 250;
static constexpr uint16_t IMAGE_HEIGHT = 122;
static constexpr size_t IMAGE_BYTES = ((uint32_t)IMAGE_WIDTH * IMAGE_HEIGHT + 7) / 8; // 3813

// RTC memory: sopravvive al deep sleep
RTC_DATA_ATTR float g_currentTime = 0.0f;
RTC_DATA_ATTR bool g_displayReady = false;

static volatile float s_receivedTime = -1.0f;
static volatile bool s_hasNewTime = false;
static volatile bool s_bleConnected = false;

// Buffer immagine e stato di ricezione
static uint8_t s_imageBuf[IMAGE_BYTES];
static size_t s_imageReceived = 0;
static uint16_t s_imageExpected = 0;
static volatile bool s_hasNewImage = false;

static SemaphoreHandle_t s_displayMutex = nullptr;

// ── Decode byte ──────────────────────────────────────
static float decodeTimeByte(uint8_t b)
{
  uint8_t quarters = (b >> 0) & 0x03;
  uint8_t hours = (b >> 2) & 0x1F;
  const float qMap[4] = {0.0f, 25.0f, 50.0f, 75.0f};
  return (float)hours + qMap[quarters] / 100.0f;
}

// ── Wrapper thread-safe per updateNumber ─────────────
static void safeUpdateNumber(float t)
{
  if (xSemaphoreTake(s_displayMutex, pdMS_TO_TICKS(5000)) == pdTRUE)
  {
    updateNumber(t);
    xSemaphoreGive(s_displayMutex);
  }
  else
  {
    Serial.println("Display: timeout mutex, skip aggiornamento");
  }
}

// ── Wrapper thread-safe per drawImage ────────────────
static void safeDrawImage(const uint8_t *data, size_t len)
{
  if (xSemaphoreTake(s_displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE)
  {
    drawImage(data, len);
    xSemaphoreGive(s_displayMutex);
  }
  else
  {
    Serial.println("Display: timeout mutex, skip drawImage");
  }
}

// ── Callbacks connessione ─────────────────────────────
class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    s_bleConnected = true;
    Serial.println("BLE: client connesso");
  }
  void onDisconnect(BLEServer *pServer) override
  {
    s_bleConnected = false;
    Serial.println("BLE: client disconnesso");
  }
};

// ── BLE Characteristic Callbacks ─────────────────────
class TimeCharCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pChar) override
  {
    std::string val = pChar->getValue();
    if (val.length() < 1)
      return;

    uint8_t firstByte = (uint8_t)val[0];

    if (firstByte & 0x80)
    {
      // ── Pacchetto immagine ────────────────────────
      switch (firstByte)
      {
      case 0x80: // START [0x80][lenHi][lenLo]
        if (val.length() >= 3)
        {
          s_imageExpected = ((uint16_t)(uint8_t)val[1] << 8) | (uint8_t)val[2];
          s_imageReceived = 0;
          s_hasNewImage = false;
          memset(s_imageBuf, 0, IMAGE_BYTES);
          Serial.printf("BLE IMG: START, expected %u bytes\n", s_imageExpected);
        }
        break;

      case 0x81: // DATA [0x81][offHi][offLo][...payload...]
        if (val.length() >= 4)
        {
          uint16_t offset = ((uint16_t)(uint8_t)val[1] << 8) | (uint8_t)val[2];
          size_t payloadLen = val.length() - 3;
          if ((size_t)offset + payloadLen <= IMAGE_BYTES)
          {
            memcpy(&s_imageBuf[offset], &val[3], payloadLen);
            s_imageReceived += payloadLen;
          }
          else
          {
            Serial.printf("BLE IMG: DATA overflow at offset %u len %u\n", offset, (unsigned)payloadLen);
          }
        }
        break;

      case 0x82: // END [0x82]
        if (s_imageReceived == s_imageExpected && s_imageExpected > 0)
        {
          Serial.printf("BLE IMG: END OK, %u bytes\n", s_imageReceived);
          safeDrawImage(s_imageBuf, IMAGE_BYTES); // ← disegna subito, senza aspettare disconnect
          s_hasNewImage = false;                  // già gestito, il fallback non ridisegna
        }
        else
        {
          Serial.printf("BLE IMG: END mismatch received=%u expected=%u\n",
                        (unsigned)s_imageReceived, s_imageExpected);
        }
        break;

      default:
        Serial.printf("BLE IMG: opcode sconosciuto 0x%02x\n", firstByte);
        break;
      }
    }
    else
    {
      // ── Pacchetto orario (comportamento originale) ─
      float t = decodeTimeByte(firstByte);
      s_receivedTime = t;
      s_hasNewTime = true;
      Serial.printf("BLE: ricevuto %.2f\n", t);
      safeUpdateNumber(t);
    }
  }
};

// ── BLE: avvia e aspetta ──────────────────────────────
static void runBLEWindow()
{
  BLEDevice::init("ParkingDisk UwU");
  BLEDevice::setMTU(247);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(
      BLE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE);
  pChar->addDescriptor(new BLE2902());
  pChar->setCallbacks(new TimeCharCallbacks());
  pService->start();

  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(BLE_SERVICE_UUID);
  pAdv->setMinInterval(800);  // 800 * 0.625ms = 500ms
  pAdv->setMaxInterval(1600); // 1600 * 0.625ms = 1000ms
  pAdv->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE: advertising...");

  unsigned long start = millis();
  while (true)
  {
    bool windowExpired = (millis() - start) >= (ADVERTISE_WINDOW_S * 1000UL);

    // Non connesso e finestra scaduta → esci
    if (!s_bleConnected && windowExpired)
      break;

    delay(50);
  }

  BLEDevice::stopAdvertising();
  BLEDevice::deinit(true);
}

// ── Setup ─────────────────────────────────────────────
void setup()
{
  s_displayMutex = xSemaphoreCreateMutex();

  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  esp_wifi_stop();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  bool firstBoot = (cause != ESP_SLEEP_WAKEUP_TIMER);

  if (firstBoot || !g_displayReady)
  {
    initDisplay();
    showInitialScreen();
    g_displayReady = true;
  }

  runBLEWindow();

  // Fallback: dati ricevuti ma display non ancora aggiornato
  // (es. onWrite chiamato mentre il mutex era occupato da initDisplay)
  if (s_hasNewImage)
  {
    s_hasNewImage = false;
    safeDrawImage(s_imageBuf, IMAGE_BYTES);
  }
  else if (s_hasNewTime)
  {
    g_currentTime = s_receivedTime;
    s_hasNewTime = false;
    safeUpdateNumber(g_currentTime);
  }

  Serial.printf("Sleep per %ds...\n", SLEEP_DURATION_S);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_S * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}