// main.cpp
#include <Arduino.h>
#include <esp_sleep.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "display.h"
#include "power.h"
#include <esp_wifi.h>

#define BLE_SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define BLE_CHARACTERISTIC_UUID "12345678-1234-1234-1234-123456789def"

// RTC memory: sopravvive al deep sleep
RTC_DATA_ATTR float g_currentTime = 0.0f;
RTC_DATA_ATTR bool g_displayReady = false;

static volatile float s_receivedTime = -1.0f;
static volatile bool s_hasNewTime = false;
static volatile bool s_bleConnected = false;

// ── Decode byte ──────────────────────────────────────
static float decodeTimeByte(uint8_t b)
{
  uint8_t quarters = (b >> 0) & 0x03;
  uint8_t hours = (b >> 2) & 0x1F;
  const float qMap[4] = {0.0f, 25.0f, 50.0f, 75.0f};
  return (float)hours + qMap[quarters] / 100.0f;
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
    s_receivedTime = decodeTimeByte((uint8_t)val[0]);
    s_hasNewTime = true;
    Serial.printf("BLE: ricevuto %.2f\n", s_receivedTime);
  }
};

// ── BLE: avvia e aspetta ──────────────────────────────
static void runBLEWindow()
{
  BLEDevice::init("ParkingDisk UwU");

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
  pAdv->setScanResponse(false);
  BLEDevice::startAdvertising();

  Serial.println("BLE: advertising...");

  unsigned long start = millis();
  while (true)
  {
    bool windowExpired = (millis() - start) >= (ADVERTISE_WINDOW_S * 1000UL);

    if (s_bleConnected)
    {
      // Connesso: rimane sveglio finché non si disconnette
      // Aggiorna display immediatamente se arrivano dati
      if (s_hasNewTime)
      {
        g_currentTime = s_receivedTime;
        s_hasNewTime = false;
        updateNumber(g_currentTime);
        Serial.printf("Display aggiornato → %.2f\n", g_currentTime);
      }
    }
    else
    {
      // Non connesso: esci se la finestra è scaduta
      if (windowExpired)
        break;
    }

    delay(50);
  }

  BLEDevice::stopAdvertising();
  BLEDevice::deinit(true);
}

// ── Setup ─────────────────────────────────────────────
void setup()
{
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  esp_wifi_stop();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  bool firstBoot = (cause != ESP_SLEEP_WAKEUP_TIMER);

  if (firstBoot || !g_displayReady)
  {
    initDisplay();
    g_displayReady = true;
  }

  runBLEWindow();

  // Gestisce dati ricevuti durante la finestra (non durante connessione)
  // quelli durante connessione sono già aggiornati dentro runBLEWindow()
  if (s_hasNewTime)
  {
    g_currentTime = s_receivedTime;
    updateNumber(g_currentTime);
  }

  Serial.printf("Sleep per %ds...\n", SLEEP_DURATION_S);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_S * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}