<script setup>
import { useRouter } from "vue-router";
import HeaderBar from "@/components/HeaderBar.vue";
import PrimaryButton from "@/components/PrimaryButton.vue";

const router = useRouter();
const goToParking = () => router.push("/parking-disk");
const goToDraw = () => router.push("/draw");
</script>

<template>
  <main class="page-shell">
    <HeaderBar title="Home" />
    <section class="screen">
      <div class="text-center">
        <h1>Home</h1>
        <p>Seleziona un'azione per iniziare.</p>
      </div>

      <div class="bottom-actions">
        <PrimaryButton label="Immagine" @click="goToDraw" />
        <PrimaryButton label="Disco orario" @click="goToParking" />
      </div>
    </section>
  </main>
</template>

<style scoped>
.page-shell {
  width: min(100%, 500px);
  margin: 0 auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.screen {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 20px 16px 24px;
  gap: 20px;
}

.text-center {
  text-align: center;
}

.bottom-actions {
  display: flex;
  gap: 12px;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  flex-wrap: nowrap;
  margin-top: auto;
}
</style>
