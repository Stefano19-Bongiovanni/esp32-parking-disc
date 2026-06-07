import { defineStore } from "pinia";
import { BleClient } from "@capacitor-community/bluetooth-le";

export const BLE_SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC";
export const BLE_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789DEF";

const defaultState = () => {
  return {
    devices: [],
    connectedDevice: null,
    isScanning: false,
    isConnecting: false, // NUOVA VARIABILE DI STATO
  };
};

function normalizeScanResult(result) {
  // Cerca il deviceId sia a livello nativo/diretto che dentro l'oggetto .device
  const deviceId = result.deviceId || result.device?.deviceId;
  // Fa la stessa cosa per il nome del dispositivo
  const name =
    result.name || result.device?.name || result.localName || "Unknown device";

  return {
    deviceId: deviceId,
    name: name,
    rssi: typeof result.rssi === "number" ? result.rssi : null,
    uuids: result.uuids ?? result.serviceUuids ?? [],
    raw: result,
  };
}

export const useBleStore = defineStore("ble", {
  state: defaultState,
  actions: {
    async START_SCANNING(scanTimeoutMs = 10000, autoConnect = false) {
      if (this.isScanning) {
        return;
      }

      this.devices = [];
      this.isScanning = true;

      // Guard per evitare doppia connessione se arrivano più result ravvicinati
      let autoConnecting = false;

      const addOrUpdate = async (result) => {
        // Estrai il deviceId corretto prima del controllo
        const deviceId = result?.deviceId || result?.device?.deviceId;

        if (!deviceId) {
          return; // Salta solo se manca davvero in entrambi i posti
        }

        const device = normalizeScanResult(result);
        const existing = this.devices.find(
          (d) => d.deviceId === device.deviceId,
        );

        if (existing) {
          Object.assign(existing, device);
        } else {
          this.devices.push(device);
        }

        if (autoConnect && !autoConnecting && !this.connectedDevice) {
          autoConnecting = true;
          try {
            await BleClient.stopLEScan();
          } catch (stopError) {
            console.warn("autoConnect: error stopping scan", stopError);
          } finally {
            this.isScanning = false;
          }
          await this.CONNECT(device.deviceId);
        }
      };

      try {
        await BleClient.initialize();

        await BleClient.requestLEScan(
          {
            services: [BLE_SERVICE_UUID],
            allowDuplicates: false,
          },
          (result) => {
            addOrUpdate(result);
          },
        );

        if (scanTimeoutMs > 0) {
          setTimeout(async () => {
            try {
              await BleClient.stopLEScan();
            } catch (stopError) {
              console.warn("Error stopping BLE scan", stopError);
            } finally {
              this.isScanning = false;
            }
          }, scanTimeoutMs);
        }
      } catch (error) {
        console.error("START_SCANNING error:", error);
        this.isScanning = false;
      }
    },

    async STOP_SCANNING() {
      if (!this.isScanning) {
        return;
      }

      try {
        await BleClient.stopLEScan();
      } catch (error) {
        console.warn("STOP_SCANNING error:", error);
      } finally {
        this.isScanning = false;
      }
    },

    async CONNECT(deviceId) {
      this.isConnecting = true; // INIZIO CONNESSIONE
      try {
        await BleClient.connect(deviceId, (disconnectedDeviceId) => {
          console.warn("BLE device disconnected:", disconnectedDeviceId);
          if (this.connectedDevice?.deviceId === disconnectedDeviceId) {
            this.connectedDevice = null;
          }
        });

        const device = this.devices.find((d) => d.deviceId === deviceId);
        this.connectedDevice = device ?? { deviceId, name: "Unknown device" };

        console.log("BLE connected to:", this.connectedDevice.name);
      } catch (error) {
        console.error("CONNECT error:", error);
        this.connectedDevice = null;
      } finally {
        this.isConnecting = false; // FINE CONNESSIONE (successo o errore)
      }
    },

    /**
     * Invia un valore orario (0–23.75, a passi di 0.25) all'ESP32.
     *
     * Formato del byte (identico al decodeTimeByte dell'ESP32):
     *   bit 6-2  → ore   (0–23)
     *   bit 1-0  → quarto d'ora (0=.00, 1=.25, 2=.50, 3=.75)
     *
     * @param {number} value  es. 0, 8.25, 13.5, 23.75
     */
    async SEND_TIME(value) {
      if (!this.connectedDevice) {
        console.warn("SEND_TIME: nessun dispositivo connesso");
        return;
      }

      // Clamp al range valido
      const clamped = Math.max(0, Math.min(23.75, value));

      const hours = Math.floor(clamped);
      const fraction = clamped - hours;

      // Trova il quarter index più vicino
      const quarterMap = [0.0, 0.25, 0.5, 0.75];
      const quarterIndex = quarterMap.reduce(
        (bestIdx, q, idx) =>
          Math.abs(fraction - q) < Math.abs(fraction - quarterMap[bestIdx])
            ? idx
            : bestIdx,
        0,
      );

      // Encode: ore nei bit 6-2, quarter nei bit 1-0
      const byte = ((hours & 0x1f) << 2) | (quarterIndex & 0x03);

      try {
        await BleClient.write(
          this.connectedDevice.deviceId,
          BLE_SERVICE_UUID,
          BLE_CHARACTERISTIC_UUID,
          new Uint8Array([byte]),
        );
        console.log(
          `SEND_TIME: ${hours + quarterMap[quarterIndex]} → 0x${byte.toString(16).padStart(2, "0")}`,
        );
      } catch (error) {
        console.error("SEND_TIME error:", error);
      }
    },

    async DISCONNECT() {
      if (!this.connectedDevice) {
        return;
      }

      try {
        await BleClient.disconnect(this.connectedDevice.deviceId);
      } catch (error) {
        console.warn("DISCONNECT error:", error);
      } finally {
        this.connectedDevice = null;
      }
    },
  },
});
