import { Capacitor } from "@capacitor/core";
import { BleClient } from "@capacitor-community/bluetooth-le";
import { SafeArea } from "capacitor-plugin-safe-area";

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

export async function set_safe_area() {
  const { insets } = await SafeArea.getSafeAreaInsets();
  //DEV ONLY
  // if (process.env.NODE_ENV === "development") {
  //   insets.top = 59;
  //   insets.right = 0;
  //   insets.bottom = 34;
  //   insets.left = 0;
  // }
  // console.log("SafeAreaInsets", insets);

  const { top, bottom, right, left } = insets;

  document.documentElement.style.setProperty(
    "--safe-area-inset-top",
    `${top}px`,
  );
  document.documentElement.style.setProperty(
    "--safe-area-inset-bottom",
    `${bottom}px`,
  );
  document.documentElement.style.setProperty(
    "--safe-area-inset-right",
    `${right}px`,
  );
  document.documentElement.style.setProperty(
    "--safe-area-inset-left",
    `${left}px`,
  );
}
