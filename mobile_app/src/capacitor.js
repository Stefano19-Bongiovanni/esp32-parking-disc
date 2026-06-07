import { Capacitor } from "@capacitor/core";
import { BleClient } from "@capacitor-community/bluetooth-le";

/**
 * Initialize BLE and request the required permissions on app startup.
 *
 * On Android this may request location permissions and prompt to enable Bluetooth.
 * On iOS this will request BLE access permissions.
 *
 * @returns {Promise<boolean>} true when BLE is initialized and ready, false otherwise.
 */
export async function initBle() {
  try {
    if (Capacitor.getPlatform() === "android") {
      const isLocationEnabled = await BleClient.isLocationEnabled();
      if (!isLocationEnabled) {
        await BleClient.openLocationSettings();
      }
    }

    await BleClient.initialize();

    if (Capacitor.getPlatform() !== "web") {
      const enabled = await BleClient.isEnabled();
      if (!enabled) {
        await BleClient.requestEnable();
      }
    }

    return true;
  } catch (error) {
    console.error("initBle error:", error);
    return false;
  }
}
