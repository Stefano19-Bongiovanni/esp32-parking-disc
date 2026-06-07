<script setup>
import { onMounted } from "vue";
import { initBle, set_safe_area } from "./capacitor";

onMounted(async () => {
  await set_safe_area();
  const ok = await initBle();
  if (!ok) {
    console.warn("BLE initialization failed.");
  }
});
</script>

<template>
  <RouterView />
</template>
