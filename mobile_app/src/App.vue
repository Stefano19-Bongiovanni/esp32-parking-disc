<script setup>
import { onMounted } from "vue";
import { initBle } from "./capacitor";

onMounted(async () => {
  const ok = await initBle();
  if (!ok) {
    console.warn("BLE initialization failed.");
  }
});
</script>

<template>
  <RouterView />
</template>
