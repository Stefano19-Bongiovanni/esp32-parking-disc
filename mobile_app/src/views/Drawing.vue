<script setup>
import { ref, computed, watch, nextTick } from "vue";
import HeaderBar from "@/components/HeaderBar.vue";
import { useBleStore } from "@/stores/ble.store";

const bleStore = useBleStore();

const CANVAS_W = 250;
const CANVAS_H = 122;
const BLACK = "#000000";
const WHITE = "#ffffff";
const PEN_WIDTH = 3;
const ERASER_WIDTH = 12;
const TEXT_SIZE = 14;

const tool = ref("pen");
const lines = ref([]);
const texts = ref([]);
const textDraft = ref("");
const imageNode = ref(null);
const isDrawing = ref(false);
const isSending = ref(false);
const sendSuccess = ref(false);
const sendError = ref(false);

const stageRef = ref(null);
const transformerRef = ref(null);
const imageRef = ref(null);
const fileInputRef = ref(null);

let textIdCounter = 0;

const stageConfig = { width: CANVAS_W, height: CANVAS_H };

const transformerConfig = {
  rotateEnabled: false,
  enabledAnchors: ["top-left", "top-right", "bottom-left", "bottom-right"],
  anchorSize: 18,
  anchorCornerRadius: 9,
  anchorFill: WHITE,
  anchorStroke: "#1a1a2e",
  borderStroke: "#1a1a2e",
  borderStrokeWidth: 2,
  padding: 4,
  boundBoxFunc: imageBoundBox,
};

const tools = [
  { id: "pen", label: "Penna" },
  { id: "text", label: "Testo" },
  { id: "image", label: "Immagine" },
  { id: "eraser", label: "Gomma" },
  { id: "clear", label: "Cancella" },
];

function imageBoundBox(oldBox, newBox) {
  if (newBox.width < 20 || newBox.height < 20) return oldBox;
  if (newBox.x < 0 || newBox.y < 0) return oldBox;
  if (newBox.x + newBox.width > CANVAS_W) return oldBox;
  if (newBox.y + newBox.height > CANVAS_H) return oldBox;
  return newBox;
}

function lineConfig(line) {
  const isEraser = line.tool === "eraser";
  return {
    points: line.points,
    stroke: isEraser ? "#000000" : BLACK,
    strokeWidth: isEraser ? ERASER_WIDTH : PEN_WIDTH,
    tension: 0.5,
    lineCap: "round",
    lineJoin: "round",
    globalCompositeOperation: isEraser ? "destination-out" : "source-over",
  };
}

function textConfig(t) {
  return {
    x: t.x,
    y: t.y,
    text: t.text,
    fontSize: TEXT_SIZE,
    fontFamily: "Courier New, monospace",
    fill: BLACK,
    draggable: tool.value === "text",
  };
}

function getStage() {
  return stageRef.value?.getNode();
}

function getPointerPos(e) {
  const stage = getStage();
  if (!stage) return null;
  return stage.getPointerPosition();
}

function onPointerDown(e) {
  if (e.target !== e.target.getStage()) return;

  if (tool.value === "text") {
    if (!textDraft.value.trim()) return;
    const pos = getPointerPos(e);
    if (!pos) return;
    texts.value.push({
      id: ++textIdCounter,
      x: pos.x,
      y: pos.y,
      text: textDraft.value.trim(),
    });
    textDraft.value = "";
    return;
  }

  if (tool.value !== "pen" && tool.value !== "eraser") return;

  isDrawing.value = true;
  const pos = getPointerPos(e);
  if (!pos) return;
  lines.value.push({ tool: tool.value, points: [pos.x, pos.y] });
}

function onPointerMove(e) {
  if (!isDrawing.value) return;
  e.evt.preventDefault();

  const pos = getPointerPos(e);
  if (!pos) return;

  const lastLine = lines.value[lines.value.length - 1];
  lastLine.points = lastLine.points.concat([pos.x, pos.y]);
  lines.value.splice(lines.value.length - 1, 1, { ...lastLine });
}

function onPointerUp() {
  isDrawing.value = false;
}

function selectTool(id) {
  if (id === "clear") {
    handleClear();
    return;
  }
  if (id === "image") {
    const shouldOpenPicker = tool.value === "image" || !imageNode.value;
    tool.value = "image";
    if (shouldOpenPicker) fileInputRef.value?.click();
    nextTick(updateTransformer);
    return;
  }
  tool.value = id;
  nextTick(updateTransformer);
}

function handleClear() {
  if (!confirm("Cancellare tutto il disegno?")) return;
  lines.value = [];
  texts.value = [];
  imageNode.value = null;
  updateTransformer();
}

function thresholdImageData(imageData) {
  const data = imageData.data;
  for (let i = 0; i < data.length; i += 4) {
    const lum = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
    const val = lum < 128 ? 0 : 255;
    data[i] = data[i + 1] = data[i + 2] = val;
    data[i + 3] = 255;
  }
  return imageData;
}

function loadBwImage(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => {
      const img = new Image();
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = CANVAS_W;
        canvas.height = CANVAS_H;
        const ctx = canvas.getContext("2d");
        ctx.fillStyle = WHITE;
        ctx.fillRect(0, 0, CANVAS_W, CANVAS_H);

        const scale = Math.min(CANVAS_W / img.width, CANVAS_H / img.height);
        const w = img.width * scale;
        const h = img.height * scale;
        const x = (CANVAS_W - w) / 2;
        const y = (CANVAS_H - h) / 2;
        ctx.drawImage(img, x, y, w, h);

        const imageData = thresholdImageData(
          ctx.getImageData(0, 0, CANVAS_W, CANVAS_H),
        );
        ctx.putImageData(imageData, 0, 0);

        const bwImg = new Image();
        bwImg.onload = () => resolve({ image: bwImg, width: w, height: h, x, y });
        bwImg.onerror = reject;
        bwImg.src = canvas.toDataURL();
      };
      img.onerror = reject;
      img.src = reader.result;
    };
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

async function onFileSelected(e) {
  const file = e.target.files?.[0];
  e.target.value = "";
  if (!file) return;

  try {
    const { image, width, height, x, y } = await loadBwImage(file);
    imageNode.value = {
      id: "img",
      name: "userImage",
      image,
      x,
      y,
      width,
      height,
      scaleX: 1,
      scaleY: 1,
    };
    tool.value = "image";
    await nextTick();
    updateTransformer();
  } catch (err) {
    console.error("Errore caricamento immagine:", err);
  }
}

function onImageTransformEnd(e) {
  const node = e.target;
  imageNode.value = {
    ...imageNode.value,
    x: node.x(),
    y: node.y(),
    width: Math.max(20, node.width() * node.scaleX()),
    height: Math.max(20, node.height() * node.scaleY()),
    scaleX: 1,
    scaleY: 1,
  };
  node.scaleX(1);
  node.scaleY(1);
}

function onImageDragEnd(e) {
  if (!imageNode.value) return;
  imageNode.value = {
    ...imageNode.value,
    x: e.target.x(),
    y: e.target.y(),
  };
}

function onTextDragEnd(e, id) {
  const t = texts.value.find((item) => item.id === id);
  if (!t) return;
  t.x = e.target.x();
  t.y = e.target.y();
}

function updateTransformer() {
  const tr = transformerRef.value?.getNode();
  const img = imageRef.value?.getNode();
  if (!tr) return;

  if (tool.value === "image" && img) {
    tr.nodes([img]);
  } else {
    tr.nodes([]);
  }
  tr.getLayer()?.batchDraw();
}

watch(tool, () => nextTick(updateTransformer));
watch(imageNode, () => nextTick(updateTransformer));

function exportImageData() {
  const tr = transformerRef.value?.getNode();
  if (tr) tr.nodes([]);

  const stage = getStage();
  const canvas = stage.toCanvas({ pixelRatio: 1 });
  const imageData = canvas
    .getContext("2d")
    .getImageData(0, 0, CANVAS_W, CANVAS_H);

  updateTransformer();
  return imageData;
}

async function handleSend() {
  if (isSending.value) return;
  isSending.value = true;
  sendSuccess.value = false;
  sendError.value = false;

  try {
    await bleStore.SEND_IMAGE(exportImageData());
    sendSuccess.value = true;
    setTimeout(() => (sendSuccess.value = false), 2500);
  } catch {
    sendError.value = true;
    setTimeout(() => (sendError.value = false), 2500);
  } finally {
    isSending.value = false;
  }
}

const imageDraggable = computed(() => tool.value === "image");
</script>

<template>
  <main class="page-shell">
    <HeaderBar title="Immagine" :showBackButton="true" />

    <section class="screen">
      <!-- Toolbar -->
      <div class="toolbar">
        <button
          v-for="t in tools"
          :key="t.id"
          type="button"
          class="tool-btn"
          :class="{ 'tool-btn--active': t.id !== 'clear' && tool === t.id }"
          @click="selectTool(t.id)"
        >
          <svg
            v-if="t.id === 'pen'"
            class="tool-icon"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M12 20h9" />
            <path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z" />
          </svg>
          <svg
            v-else-if="t.id === 'text'"
            class="tool-icon"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
          >
            <polyline points="4 7 4 4 20 4 20 7" />
            <line x1="9" y1="20" x2="15" y2="20" />
            <line x1="12" y1="4" x2="12" y2="20" />
          </svg>
          <svg
            v-else-if="t.id === 'image'"
            class="tool-icon"
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
          <svg
            v-else-if="t.id === 'eraser'"
            class="tool-icon"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path
              d="M20 20H7L3 16a2.12 2.12 0 0 1 0-3L13 3a2.12 2.12 0 0 1 3 0l7 7a2.12 2.12 0 0 1 0 3Z"
            />
            <line x1="18" y1="13" x2="13" y2="18" />
          </svg>
          <svg
            v-else
            class="tool-icon"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
          >
            <polyline points="3 6 5 6 21 6" />
            <path
              d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"
            />
          </svg>
          <span class="tool-label">{{ t.label }}</span>
        </button>
      </div>

      <input
        ref="fileInputRef"
        type="file"
        accept="image/*"
        hidden
        @change="onFileSelected"
      />

      <!-- Canvas -->
      <div class="canvas-card">
        <div class="canvas-scaler">
          <v-stage
            ref="stageRef"
            :config="stageConfig"
            @mousedown="onPointerDown"
            @mousemove="onPointerMove"
            @mouseup="onPointerUp"
            @touchstart="onPointerDown"
            @touchmove="onPointerMove"
            @touchend="onPointerUp"
          >
            <v-layer>
              <v-rect
                :config="{
                  x: 0,
                  y: 0,
                  width: CANVAS_W,
                  height: CANVAS_H,
                  fill: WHITE,
                }"
              />
              <v-image
                v-if="imageNode"
                ref="imageRef"
                :config="imageNode"
                :draggable="imageDraggable"
                @transformend="onImageTransformEnd"
                @dragend="onImageDragEnd"
              />
              <v-line
                v-for="(line, i) in lines"
                :key="'l' + i"
                :config="lineConfig(line)"
              />
              <v-text
                v-for="t in texts"
                :key="t.id"
                :config="textConfig(t)"
                @dragend="(e) => onTextDragEnd(e, t.id)"
              />
              <v-transformer ref="transformerRef" :config="transformerConfig" />
            </v-layer>
          </v-stage>
        </div>
      </div>

      <!-- Barra testo (solo in modalità testo) -->
      <div v-if="tool === 'text'" class="text-bar">
        <input
          v-model="textDraft"
          class="text-input"
          type="text"
          placeholder="Scrivi il testo…"
          maxlength="40"
        />
        <p class="text-hint">Scrivi il testo, poi tocca il canvas per posizionarlo</p>
      </div>

      <div class="spacer" />

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
          Immagine inviata
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
          Invia immagine
        </span>
      </button>

      <p v-if="!bleStore.connectedDevice" class="no-device-hint">
        Nessun dispositivo connesso
      </p>
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
  background: #f7f4ef;
}

.screen {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 16px 16px calc(24px + var(--safe-area-inset-bottom));
  gap: 16px;
}

/* ── Toolbar ─────────────────────────────────────────────────────────────── */
.toolbar {
  display: flex;
  gap: 8px;
  overflow-x: auto;
  -webkit-overflow-scrolling: touch;
  padding-bottom: 4px;
}

.tool-btn {
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4px;
  min-width: 56px;
  min-height: 56px;
  padding: 6px 8px;
  border: 1.5px solid rgba(26, 26, 46, 0.12);
  border-radius: 14px;
  background: #ffffff;
  color: #1a1a2e;
  cursor: pointer;
  transition: background 0.15s, transform 0.1s;
}

.tool-btn:active {
  transform: scale(0.95);
}

.tool-btn--active {
  background: #1a1a2e;
  color: #f7f4ef;
  border-color: #1a1a2e;
}

.tool-icon {
  width: 24px;
  height: 24px;
  flex-shrink: 0;
}

.tool-label {
  font-family: "Courier New", monospace;
  font-size: 0.62rem;
  font-weight: 600;
  letter-spacing: 0.02em;
  white-space: nowrap;
}

/* ── Canvas card ─────────────────────────────────────────────────────────── */
.canvas-card {
  background: #ffffff;
  border-radius: 20px;
  box-shadow:
    0 2px 8px rgba(0, 0, 0, 0.06),
    0 0 0 1.5px rgba(26, 26, 46, 0.07);
  padding: 16px;
  display: flex;
  justify-content: center;
}

.canvas-scaler {
  width: 100%;
  touch-action: none;
  overscroll-behavior: contain;
  -webkit-user-select: none;
  user-select: none;
}

.canvas-scaler :deep(canvas) {
  width: 100% !important;
  height: auto !important;
  display: block;
}

/* ── Barra testo ───────────────────────────────────────────────────────────── */
.text-bar {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.text-input {
  width: 100%;
  padding: 12px 14px;
  border: 1.5px solid rgba(26, 26, 46, 0.15);
  border-radius: 12px;
  font-size: 16px;
  font-family: "Courier New", monospace;
  background: #ffffff;
  color: #1a1a2e;
  outline: none;
}

.text-input:focus {
  border-color: #1a1a2e;
}

.text-hint {
  margin: 0;
  font-size: 0.75rem;
  color: #888;
  font-family: "Courier New", monospace;
}

.spacer {
  flex: 1;
}

/* ── Bottone invio ─────────────────────────────────────────────────────────── */
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

.send-btn--success:disabled {
  opacity: 1;
}

.send-btn--error {
  background: #8b1a1a;
  box-shadow: 0 4px 16px rgba(139, 26, 26, 0.3);
}

.send-btn--error:disabled {
  opacity: 1;
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

.spinner {
  animation: spin 0.9s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.no-device-hint {
  text-align: center;
  font-family: "Courier New", monospace;
  font-size: 0.8rem;
  color: #aaa;
  margin-top: -8px;
}
</style>
