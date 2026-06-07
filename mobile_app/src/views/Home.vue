<script setup>
import { computed } from "vue";
import { useRouter } from "vue-router";
import { useBleStore } from "@/stores/ble.store";
import HeaderBar from "@/components/HeaderBar.vue";

const router = useRouter();
const bleStore = useBleStore();

const goToParking = () => router.push("/parking-disk");
const goToDraw = () => router.push("/draw");

const toggleConnection = async () => {
  if (bleStore.connectedDevice) {
    await bleStore.DISCONNECT();
  } else if (!bleStore.isScanning) {
    await bleStore.START_SCANNING(40000, true);
  }
};

const isConnected = computed(() => !!bleStore.connectedDevice);
const isScanning = computed(() => bleStore.isScanning);
const deviceName = computed(
  () => bleStore.connectedDevice?.name || "Dispositivo",
);
</script>

<template>
  <main class="page-shell">
    <HeaderBar title="Home" />

    <section class="screen">
      <!-- Intestazione e stato -->
      <div class="status-header">
        <h1>Benvenuto</h1>
        <transition name="fade" mode="out-in">
          <p v-if="isConnected" class="status text-success" key="connected">
            <span class="dot pulse-green"></span> Connesso a {{ deviceName }}
          </p>
          <p v-else-if="isScanning" class="status text-warning" key="scanning">
            <span class="dot pulse-yellow"></span> Ricerca dispositivo...
          </p>
          <p v-else class="status text-muted" key="disconnected">
            <span class="dot"></span> Nessun dispositivo connesso
          </p>
        </transition>
      </div>

      <!-- Area Interattiva Centrale (Dimensione fissa in cui avviene la magia) -->
      <div class="interactive-area">
        <transition name="spring" mode="out-in">
          <!-- STATO 1: Pulsante Unico Gigante (Non Connesso) -->
          <button
            v-if="!isConnected"
            class="hero-btn connect-btn"
            :class="{ 'is-scanning': isScanning }"
            @click="toggleConnection"
            key="connect-state"
          >
            <!-- Effetto Radar quando scansiona -->
            <div v-if="isScanning" class="radar-rings">
              <div class="ring ring-1"></div>
              <div class="ring ring-2"></div>
            </div>

            <div class="hero-content">
              <svg
                v-if="!isScanning"
                class="hero-icon"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="M6.5 6.5l11 11L12 23V1l5.5 5.5-11 11" />
              </svg>
              <svg
                v-else
                class="hero-icon spin"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <line x1="12" y1="2" x2="12" y2="6"></line>
                <line x1="12" y1="18" x2="12" y2="22"></line>
                <line x1="4.93" y1="4.93" x2="7.76" y2="7.76"></line>
                <line x1="16.24" y1="16.24" x2="19.07" y2="19.07"></line>
                <line x1="2" y1="12" x2="6" y2="12"></line>
                <line x1="18" y1="12" x2="22" y2="12"></line>
                <line x1="4.93" y1="19.07" x2="7.76" y2="16.24"></line>
                <line x1="16.24" y1="7.76" x2="19.07" y2="4.93"></line>
              </svg>
              <h2>{{ isScanning ? "RICERCA..." : "CONNETTI" }}</h2>
              <p v-if="!isScanning">Tocca per associare il dispositivo</p>
            </div>
          </button>

          <!-- STATO 2: Pulsanti Divisi (Connesso) -->
          <div v-else class="split-actions" key="actions-state">
            <button class="hero-btn split-btn draw-btn" @click="goToDraw">
              <div class="hero-content horizontal">
                <div class="icon-wrapper">
                  <svg
                    class="hero-icon"
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  >
                    <rect x="3" y="3" width="18" height="18" rx="2" ry="2" />
                    <circle cx="8.5" cy="8.5" r="1.5" />
                    <polyline points="21 15 16 10 5 21" />
                  </svg>
                </div>
                <div class="text-wrapper">
                  <h2>Immagine</h2>
                  <p>Invia grafica</p>
                </div>
              </div>
            </button>

            <button class="hero-btn split-btn parking-btn" @click="goToParking">
              <div class="hero-content horizontal">
                <div class="icon-wrapper">
                  <svg
                    class="hero-icon"
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                  >
                    <circle cx="12" cy="12" r="10" />
                    <polyline points="12 6 12 12 16 14" />
                  </svg>
                </div>
                <div class="text-wrapper">
                  <h2>Disco Orario</h2>
                  <p>Imposta orario</p>
                </div>
              </div>
            </button>
          </div>
        </transition>
      </div>

      <!-- Pulsante per disconnettersi -->
      <transition name="fade">
        <div class="bottom-actions" v-if="isConnected">
          <button class="disconnect-link" @click="toggleConnection">
            Disconnetti {{ deviceName }}
          </button>
        </div>
      </transition>
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
  background-color: #f8f9fa; /* Colore di sfondo base chiaro e pulito */
  font-family:
    -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial,
    sans-serif;
}

.screen {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 32px 20px;
  gap: 40px;
}

/* === INTESTAZIONE === */
.status-header {
  text-align: center;
  margin-top: 20px;
}

.status-header h1 {
  font-size: 28px;
  font-weight: 800;
  color: #1a1a2e;
  margin: 0 0 8px 0;
}

.status {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  font-size: 15px;
  font-weight: 500;
  margin: 0;
}

.dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background-color: #94a3b8;
}
.text-success {
  color: #10b981;
}
.text-success .dot {
  background-color: #10b981;
  box-shadow: 0 0 12px rgba(16, 185, 129, 0.6);
}
.text-warning {
  color: #f59e0b;
}
.text-warning .dot {
  background-color: #f59e0b;
}
.text-muted {
  color: #64748b;
}

/* === L'AREA MAGICA (Mantiene le proporzioni fisse) === */
.interactive-area {
  width: 100%;
  max-width: 340px;
  aspect-ratio: 1 / 1; /* Un quadrato perfetto per il mobile */
  position: relative;
  perspective: 1000px;
}

/* === STILI BOTTONI BASE === */
.hero-btn {
  width: 100%;
  height: 100%;
  border-radius: 36px;
  border: none;
  cursor: pointer;
  position: relative;
  overflow: hidden;
  box-shadow: 0 20px 40px -10px rgba(0, 0, 0, 0.15);
  transition:
    transform 0.2s ease,
    box-shadow 0.2s ease,
    filter 0.2s ease;
  color: white;
}

.hero-btn:active {
  transform: scale(0.96);
  box-shadow: 0 10px 20px -10px rgba(0, 0, 0, 0.1);
}

.hero-content {
  position: relative;
  z-index: 2;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  gap: 12px;
}

.hero-icon {
  width: 48px;
  height: 48px;
  opacity: 0.9;
}

.hero-btn h2 {
  font-size: 24px;
  font-weight: 800;
  letter-spacing: 1px;
  margin: 0;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

.hero-btn p {
  font-size: 14px;
  font-weight: 500;
  opacity: 0.8;
  margin: 0;
}

/* === PULSANTE CONNETTI GIGANTE === */
.connect-btn {
  background: linear-gradient(135deg, #4f46e5 0%, #7c3aed 100%);
}

.connect-btn.is-scanning {
  background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
}

/* Effetto Radar */
.radar-rings {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 100%;
  height: 100%;
  z-index: 1;
}

.ring {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 50px;
  height: 50px;
  border-radius: 50%;
  border: 2px solid rgba(255, 255, 255, 0.6);
  animation: radar-pulse 2s cubic-bezier(0.165, 0.84, 0.44, 1) infinite;
}

.ring-2 {
  animation-delay: 1s;
}

/* === STATO CONNESSO: DIVISO IN DUE === */
.split-actions {
  display: flex;
  flex-direction: column;
  gap: 16px; /* Spazio tra i due bottoni */
  height: 100%;
  width: 100%;
}

.split-btn {
  flex: 1;
  border-radius: 28px;
}

/* Rende il layout del testo orizzontale per i bottoni divisi */
.hero-content.horizontal {
  flex-direction: row;
  justify-content: flex-start;
  padding: 0 24px;
  gap: 20px;
}

.split-btn .hero-icon {
  width: 36px;
  height: 36px;
}

.split-btn h2 {
  font-size: 20px;
  text-align: left;
}

.split-btn p {
  text-align: left;
}

.text-wrapper {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
}

.draw-btn {
  background: linear-gradient(135deg, #f43f5e 0%, #fb923c 100%);
}

.parking-btn {
  background: linear-gradient(135deg, #0ea5e9 0%, #2563eb 100%);
}

/* === DISCONNETTI === */
.bottom-actions {
  margin-top: auto;
}

.disconnect-link {
  background: transparent;
  border: none;
  color: #ef4444;
  font-weight: 600;
  font-size: 16px;
  padding: 12px 24px;
  cursor: pointer;
  border-radius: 99px;
  transition: background-color 0.2s;
}

.disconnect-link:active {
  background-color: rgba(239, 68, 68, 0.1);
}

/* === ANIMAZIONI (Vue Transitions) === */
.spring-enter-active,
.spring-leave-active {
  transition: all 0.5s cubic-bezier(0.34, 1.56, 0.64, 1);
  position: absolute; /* Evita salti di layout durante lo swap */
  width: 100%;
  height: 100%;
}

.spring-enter-from {
  opacity: 0;
  transform: scale(0.8) translateY(20px);
}

.spring-leave-to {
  opacity: 0;
  transform: scale(0.8) translateY(-20px);
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* Keyframes */
@keyframes radar-pulse {
  0% {
    width: 50px;
    height: 50px;
    opacity: 1;
  }
  100% {
    width: 400px;
    height: 400px;
    opacity: 0;
  }
}

@keyframes spin {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.spin {
  animation: spin 3s linear infinite;
}
</style>
