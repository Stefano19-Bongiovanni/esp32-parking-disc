<script setup>
import { ref, onMounted, watch } from "vue";
import HeaderBar from "@/components/HeaderBar.vue";
import { useBleStore } from "@/stores/ble.store";

const bleStore = useBleStore();

// ─── Stato ────────────────────────────────────────────────────────────────────
const currentValue = ref(0);
const isSending = ref(false);
const sendSuccess = ref(false);
const sendError = ref(false);

// ─── Canvas constants (identici al demo) ─────────────────────────────────────
const R = 400;
const fontSize = 80;
const topOffset = 16;
const tickSizes = { large: [4, 20], medium: [2, 14], small: [2, 6] };
const angleSpace = 360 / 24;
const ticks = [
  { size: "small", angle: -(7 * angleSpace) / 4 },
  { size: "medium", angle: -(6 * angleSpace) / 4 },
  { size: "small", angle: -(5 * angleSpace) / 4 },
  { size: "large", angle: -(4 * angleSpace) / 4 },
  { size: "small", angle: -(3 * angleSpace) / 4 },
  { size: "medium", angle: -(2 * angleSpace) / 4 },
  { size: "small", angle: -angleSpace / 4 },
  { size: "large", angle: 0 },
  { size: "small", angle: angleSpace / 4 },
  { size: "medium", angle: (2 * angleSpace) / 4 },
  { size: "small", angle: (3 * angleSpace) / 4 },
  { size: "large", angle: (4 * angleSpace) / 4 },
  { size: "small", angle: (5 * angleSpace) / 4 },
  { size: "medium", angle: (6 * angleSpace) / 4 },
  { size: "small", angle: (7 * angleSpace) / 4 },
];

// ─── Canvas refs ──────────────────────────────────────────────────────────────
const canvasRef = ref(null);

function wrap(n) {
  return ((n % 24) + 24) % 24;
}

function getCanvas() {
  return canvasRef.value;
}

function getCtx() {
  return getCanvas()?.getContext("2d");
}

function getOrigin(canvas) {
  return {
    x: canvas.width / 2,
    y: canvas.height / 2 - R + topOffset,
  };
}

function angleToCoordinates(origin, angle, radiusOffset = 0) {
  return {
    x: origin.x + (R + radiusOffset) * Math.cos((angle * Math.PI) / 180),
    y: origin.y + (R + radiusOffset) * Math.sin((angle * Math.PI) / 180),
  };
}

function drawText(ctx, text, x, y, r = 0) {
  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(r);
  ctx.fillStyle = "#1a1a2e";
  ctx.font = `bold ${fontSize}px 'Courier New', monospace`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(text, 0, 0);
  ctx.restore();
}

function drawAngledText(ctx, origin, text, angle) {
  const { x, y } = angleToCoordinates(origin, angle);
  drawText(ctx, text, x, y, (angle * Math.PI) / 180 - Math.PI / 2);
}

function drawBackground(ctx, canvas) {
  ctx.fillStyle = "#f7f4ef";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
}

function getIntAndShift(number) {
  const value = Math.round(number);
  const decimal = number - value;
  const shift = decimal * angleSpace;
  return { value, shift };
}

function drawTick(ctx, origin, size, angle) {
  const _size = tickSizes[size];
  const { x, y } = angleToCoordinates(origin, angle, -65 + _size[1] / 2);
  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(((angle + 90) * Math.PI) / 180);
  ctx.fillStyle = "#1a1a2e";
  ctx.fillRect(-_size[0] / 2, -_size[1] / 2, _size[0], _size[1]);
  ctx.restore();
}

function drawTicks(ctx, origin, shift) {
  for (const tick of ticks) {
    drawTick(ctx, origin, tick.size, 90 + tick.angle + shift);
  }
}

function renderCanvas(val) {
  const canvas = getCanvas();
  if (!canvas) return;
  const ctx = getCtx();
  const origin = getOrigin(canvas);

  drawBackground(ctx, canvas);

  const { value, shift } = getIntAndShift(val);
  const baseAngle = 90 + shift;

  drawAngledText(ctx, origin, wrap(value), baseAngle);
  drawAngledText(ctx, origin, wrap(value + 1), baseAngle - angleSpace);
  drawAngledText(ctx, origin, wrap(value - 1), baseAngle + angleSpace);

  drawTicks(ctx, origin, shift);
}

// ─── Display label ────────────────────────────────────────────────────────────
function formatTime(val) {
  const h = Math.floor(val);
  const m = Math.round((val - h) * 60);
  return `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}`;
}

// ─── Invio BLE ────────────────────────────────────────────────────────────────
async function handleSend() {
  if (isSending.value) return;
  isSending.value = true;
  sendSuccess.value = false;
  sendError.value = false;

  try {
    await bleStore.SEND_TIME(currentValue.value);
    sendSuccess.value = true;
    setTimeout(() => (sendSuccess.value = false), 2500);
  } catch {
    sendError.value = true;
    setTimeout(() => (sendError.value = false), 2500);
  } finally {
    isSending.value = false;
  }
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────
onMounted(() => {
  renderCanvas(currentValue.value);
});

watch(currentValue, (val) => {
  renderCanvas(parseFloat(val));
});
</script>

<template>
  <main class="page-shell">
    <HeaderBar title="Disco orario" :showBackButton="true" />

    <section class="screen">
      <!-- Picker drum -->
      <div class="drum-wrapper">
        <div class="drum-card">
          <!-- Indicatore freccia -->
          <div class="arrow-indicator">
            <svg
              viewBox="0 0 40 24"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <path d="M20 24 L2 2 L38 2 Z" fill="#1a1a2e" />
            </svg>
          </div>

          <!-- Canvas del tamburo -->
          <div class="canvas-wrap">
            <canvas ref="canvasRef" width="280" height="120" />
          </div>
        </div>

        <!-- Orario selezionato -->
        <div class="time-display">
          <span class="time-label">{{ formatTime(currentValue) }}</span>
        </div>
      </div>

      <!-- Slider -->
      <div class="slider-section">
        <div class="slider-labels">
          <span>00:00</span>
          <span>12:00</span>
          <span>23:45</span>
        </div>
        <input
          class="time-slider"
          type="range"
          v-model="currentValue"
          min="0"
          max="23.75"
          step="0.25"
        />
      </div>

      <!-- Spacer -->
      <div style="flex: 1" />

      <!-- Bottone invio -->
      <button
        class="send-btn"
        :class="{
          'send-btn--loading': isSending,
          'send-btn--success': sendSuccess,
          'send-btn--error': sendError,
        }"
        :disabled="isSending || !bleStore.connectedDevice"
        @click="handleSend"
      >
        <span v-if="isSending" class="btn-inner">
          <svg class="spinner" viewBox="0 0 24 24" fill="none">
            <circle
              cx="12"
              cy="12"
              r="10"
              stroke="currentColor"
              stroke-width="2.5"
              stroke-dasharray="40"
              stroke-dashoffset="10"
            />
          </svg>
          Invio in corso…
        </span>
        <span v-else-if="sendSuccess" class="btn-inner">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.5"
            stroke-linecap="round"
          >
            <polyline points="20 6 9 17 4 12" />
          </svg>
          Orario impostato
        </span>
        <span v-else-if="sendError" class="btn-inner">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.5"
            stroke-linecap="round"
          >
            <line x1="18" y1="6" x2="6" y2="18" />
            <line x1="6" y1="6" x2="18" y2="18" />
          </svg>
          Errore invio
        </span>
        <span v-else class="btn-inner">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.5"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M22 2 L11 13" />
            <path d="M22 2 L15 22 L11 13 L2 9 Z" />
          </svg>
          Imposta orario
        </span>
      </button>

      <!-- Stato connessione -->
      <p v-if="!bleStore.connectedDevice" class="no-device-hint">
        Nessun dispositivo connesso
      </p>
    </section>
  </main>
</template>

<style scoped>
/* ── Shell ──────────────────────────────────────────────────────────────────── */
.page-shell {
  width: min(100%, 500px);
  margin: 0 auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: #f7f4ef;
}

.screen {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 24px 20px 36px;
  gap: 24px;
}

/* ── Drum wrapper ────────────────────────────────────────────────────────────── */
.drum-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
}

.drum-card {
  width: 100%;
  background: #ffffff;
  border-radius: 20px;
  box-shadow:
    0 2px 8px rgba(0, 0, 0, 0.06),
    0 0 0 1.5px rgba(26, 26, 46, 0.07);
  display: flex;
  flex-direction: column;
  align-items: center;
  padding-bottom: 16px;
  overflow: hidden;
}

/* freccia indicator */
.arrow-indicator {
  width: 40px;
  padding: 12px 0 6px;
}
.arrow-indicator svg {
  display: block;
  width: 100%;
}

/* Canvas */
.canvas-wrap {
  width: 100%;
  display: flex;
  justify-content: center;
}
.canvas-wrap canvas {
  display: block;
  max-width: 100%;
}

/* Orario grande */
.time-display {
  display: flex;
  align-items: center;
  justify-content: center;
}
.time-label {
  font-family: "Courier New", monospace;
  font-size: 3rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  color: #1a1a2e;
}

/* ── Slider ──────────────────────────────────────────────────────────────────── */
.slider-section {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.slider-labels {
  display: flex;
  justify-content: space-between;
  font-family: "Courier New", monospace;
  font-size: 0.72rem;
  color: #888;
  padding: 0 2px;
}

.time-slider {
  -webkit-appearance: none;
  appearance: none;
  width: 100%;
  height: 6px;
  border-radius: 3px;
  background: linear-gradient(
    to right,
    #1a1a2e 0%,
    #1a1a2e calc(var(--pct, 0%)),
    #d6d2cb calc(var(--pct, 0%)),
    #d6d2cb 100%
  );
  outline: none;
  cursor: pointer;
}

.time-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 26px;
  height: 26px;
  border-radius: 50%;
  background: #1a1a2e;
  border: 3px solid #f7f4ef;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.25);
  cursor: grab;
  transition: transform 0.1s;
}
.time-slider:active::-webkit-slider-thumb {
  transform: scale(1.15);
  cursor: grabbing;
}

/* ── Bottone invio ───────────────────────────────────────────────────────────── */
.send-btn {
  width: 100%;
  padding: 18px 24px;
  border: none;
  border-radius: 16px;
  background: #1a1a2e;
  color: #f7f4ef;
  font-family: "Courier New", monospace;
  font-size: 1.05rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  cursor: pointer;
  transition:
    background 0.25s,
    transform 0.1s,
    box-shadow 0.25s;
  box-shadow: 0 4px 16px rgba(26, 26, 46, 0.25);
}

.send-btn:not(:disabled):active {
  transform: scale(0.97);
  box-shadow: 0 2px 8px rgba(26, 26, 46, 0.2);
}

.send-btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
  box-shadow: none;
}

.send-btn--success {
  background: #1e6b45;
  box-shadow: 0 4px 16px rgba(30, 107, 69, 0.3);
}

.send-btn--error {
  background: #8b1a1a;
  box-shadow: 0 4px 16px rgba(139, 26, 26, 0.3);
}

.btn-inner {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
}

.btn-inner svg {
  width: 20px;
  height: 20px;
  flex-shrink: 0;
}

/* spinner rotation */
.spinner {
  animation: spin 0.9s linear infinite;
}
@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

/* ── No device hint ──────────────────────────────────────────────────────────── */
.no-device-hint {
  text-align: center;
  font-family: "Courier New", monospace;
  font-size: 0.8rem;
  color: #aaa;
  margin-top: -12px;
}
</style>
