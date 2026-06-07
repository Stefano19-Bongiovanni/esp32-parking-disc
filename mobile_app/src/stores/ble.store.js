import { defineStore } from "pinia";
import { BleClient } from "@capacitor-community/bluetooth-le";

export const BLE_SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
export const BLE_CHARACTERISTIC_UUID = "12345678-1234-1234-1234-123456789def";

const defaultState = () => {
  return {
    devices: [],
    connectedDevice: null,
    isScanning: false,
  };
};

function normalizeScanResult(result) {
  return {
    deviceId: result.deviceId,
    name: result.name || result.localName || "Unknown device",
    rssi: typeof result.rssi === "number" ? result.rssi : null,
    uuids: result.uuids ?? result.serviceUuids ?? [],
    raw: result,
  };
}

export const useBleStore = defineStore("ble", {
  state: defaultState,
  actions: {
    async START_SCANNING(scanTimeoutMs = 10000) {
      if (this.isScanning) {
        return;
      }

      this.devices = [];
      this.isScanning = true;

      const addOrUpdate = (result) => {
        if (!result?.deviceId) {
          return;
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
      };

      try {
        await BleClient.initialize();

        await BleClient.requestLEScan(
          {
            services: [BLE_SERVICE_UUID], // 👈 filtra per service UUID
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
  },
});
