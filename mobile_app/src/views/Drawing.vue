<script setup>
import { ref, computed, watch, nextTick, onMounted, onUnmounted } from "vue";
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

// Quale elemento è selezionato: { type: 'text'|'image', id }
const selectedNode = ref(null);

const stageRef = ref(null);
const transformerRef = ref(null);
const imageRef = ref(null);
const fileInputRef = ref(null);
const containerRef = ref(null);

const displayScale = ref(1);
let textIdCounter = 0;
let lastPos = null;

onMounted(() => {
  recalcDisplayScale();
  window.addEventListener("resize", recalcDisplayScale);
});
onUnmounted(() => window.removeEventListener("resize", recalcDisplayScale));

function recalcDisplayScale() {
  if (!containerRef.value) return;
  displayScale.value = containerRef.value.clientWidth / CANVAS_W;
}

const stageConfig = computed(() => ({ width: CANVAS_W, height: CANVAS_H }));

// Transformer config per IMMAGINE: stretch completo + rotazione
const imageTransformerConfig = {
  rotateEnabled: true,
  enabledAnchors: [
    "top-left", "top-center", "top-right",
    "middle-left", "middle-right",
    "bottom-left", "bottom-center", "bottom-right",
  ],
  anchorSize: 16,
  anchorCornerRadius: 8,
  anchorFill: WHITE,
  anchorStroke: "#1a1a2e",
  borderStroke: "#1a1a2e",
  borderStrokeWidth: 2,
  padding: 4,
  keepRatio: false,         // stretch libero
  boundBoxFunc: imageBoundBox,
};

// Transformer config per TESTO: ridimensiona + ruota, no stretch
const textTransformerConfig = {
  rotateEnabled: true,
  enabledAnchors: ["top-left", "top-right", "bottom-left", "bottom-right"],
  anchorSize: 16,
  anchorCornerRadius: 8,
  anchorFill: WHITE,
  anchorStroke: "#1a1a2e",
  borderStroke: "#1a1a2e",
  borderStrokeWidth: 2,
  padding: 4,
  keepRatio: true,
};

const tools = [
  { id: "pen",    label: "Penna"    },
  { id: "text",   label: "Testo"    },
  { id: "image",  label: "Immagine" },
  { id: "eraser", label: "Gomma"    },
  { id: "clear",  label: "Cancella" },
];

function imageBoundBox(oldBox, newBox) {
  if (newBox.width < 20 || newBox.height < 20) return oldBox;
  return newBox; // lascia libero il movimento fuori canvas, Konva clippa
}

function lineConfig(line) {
  return {
    points: line.points,
    stroke: line.tool === "eraser" ? WHITE : BLACK,
    strokeWidth: line.tool === "eraser" ? ERASER_WIDTH : PEN_WIDTH,
    tension: 0.5,
    lineCap: "round",
    lineJoin: "round",
    globalCompositeOperation: "source-over",
  };
}

function textConfig(t) {
  return {
    id: "text_" + t.id,
    x: t.x,
    y: t.y,
    text: t.text,
    fontSize: t.fontSize ?? TEXT_SIZE,
    fontFamily: "Courier New, monospace",
    fontStyle: "bold",
    fill: BLACK,
    draggable: tool.value === "text",
    rotation: t.rotation ?? 0,
  };
}

function getStage() {
  return stageRef.value?.getNode();
}

function getPointerPos(e) {
  const stage = getStage();
  if (!stage) return null;
  const nativeEvt = e.evt;
  const container = stage.container();
  const rect = container.getBoundingClientRect();

  let clientX, clientY;
  if (nativeEvt.touches?.length > 0) {
    clientX = nativeEvt.touches[0].clientX;
    clientY = nativeEvt.touches[0].clientY;
  } else if (nativeEvt.changedTouches?.length > 0) {
    clientX = nativeEvt.changedTouches[0].clientX;
    clientY = nativeEvt.changedTouches[0].clientY;
  } else {
    clientX = nativeEvt.clientX;
    clientY = nativeEvt.clientY;
  }

  return {
    x: (clientX - rect.left) * (CANVAS_W / rect.width),
    y: (clientY - rect.top)  * (CANVAS_H / rect.height),
  };
}

function onPointerDown(e) {
  if (tool.value === "text") {
    // click su stage vuoto → posiziona testo
    if (e.target !== getStage()) {
      // click su un testo esistente: selezionalo
      const clickedId = e.target?.attrs?.id;
      if (clickedId?.startsWith("text_")) {
        const numId = parseInt(clickedId.replace("text_", ""));
        selectedNode.value = { type: "text", id: numId };
        nextTick(updateTransformer);
      }
      return;
    }
    if (!textDraft.value.trim()) return;
    const pos = getPointerPos(e);
    if (!pos) return;
    const newId = ++textIdCounter;
    texts.value = [...texts.value, {
      id: newId,
      x: Math.min(pos.x, CANVAS_W - 10),
      y: Math.min(pos.y, CANVAS_H - TEXT_SIZE),
      text: textDraft.value.trim(),
      fontSize: TEXT_SIZE,
      rotation: 0,
    }];
    textDraft.value = "";
    selectedNode.value = { type: "text", id: newId };
    nextTick(updateTransformer);
    return;
  }

  // Deseleziona quando si disegna
  selectedNode.value = null;
  nextTick(updateTransformer);

  if (tool.value !== "pen" && tool.value !== "eraser") return;
  e.evt.preventDefault();
  isDrawing.value = true;
  const pos = getPointerPos(e);
  if (!pos) return;
  lastPos = pos;
  lines.value = [...lines.value, { tool: tool.value, points: [pos.x, pos.y, pos.x, pos.y] }];
}

function onPointerMove(e) {
  if (!isDrawing.value) return;
  e.evt.preventDefault();
  const pos = getPointerPos(e);
  if (!pos) return;
  if (lastPos && Math.abs(pos.x - lastPos.x) < 0.5 && Math.abs(pos.y - lastPos.y) < 0.5) return;
  lastPos = pos;
  const lastLine = lines.value[lines.value.length - 1];
  if (!lastLine) return;
  lines.value = [...lines.value.slice(0, -1), { ...lastLine, points: [...lastLine.points, pos.x, pos.y] }];
}

function onPointerUp(e) {
  if (isDrawing.value) e.evt?.preventDefault();
  isDrawing.value = false;
  lastPos = null;
}

function selectTool(id) {
  if (id === "clear") { handleClear(); return; }
  if (id === "image") {
    tool.value = "image";
    fileInputRef.value?.click();
    selectedNode.value = imageNode.value ? { type: "image" } : null;
    nextTick(updateTransformer);
    return;
  }
  tool.value = id;
  selectedNode.value = null;
  nextTick(updateTransformer);
}

function handleClear() {
  if (!confirm("Cancellare tutto il disegno?")) return;
  lines.value = [];
  texts.value = [];
  imageNode.value = null;
  selectedNode.value = null;
  updateTransformer();
}

// ── Immagine ────────────────────────────────────────────────────────

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
        const offscreen = document.createElement("canvas");
        offscreen.width = img.width;
        offscreen.height = img.height;
        const ctx = offscreen.getContext("2d");
        ctx.drawImage(img, 0, 0);
        const imageData = thresholdImageData(ctx.getImageData(0, 0, img.width, img.height));
        ctx.putImageData(imageData, 0, 0);
        const bwImg = new Image();
        bwImg.onload = () => {
          const scale = Math.min(CANVAS_W / img.width, CANVAS_H / img.height, 1);
          const w = Math.round(img.width * scale);
          const h = Math.round(img.height * scale);
          resolve({ image: bwImg, width: w, height: h,
            x: Math.round((CANVAS_W - w) / 2),
            y: Math.round((CANVAS_H - h) / 2) });
        };
        bwImg.onerror = reject;
        bwImg.src = offscreen.toDataURL();
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
    imageNode.value = { id: "img", name: "userImage", image, x, y, width, height, scaleX: 1, scaleY: 1, rotation: 0 };
    tool.value = "image";
    selectedNode.value = { type: "image" };
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
    width:    Math.max(20, node.width()  * node.scaleX()),
    height:   Math.max(20, node.height() * node.scaleY()),
    rotation: node.rotation(),
    scaleX: 1,
    scaleY: 1,
  };
  node.scaleX(1);
  node.scaleY(1);
}

function onImageDragEnd(e) {
  if (!imageNode.value) return;
  imageNode.value = { ...imageNode.value, x: e.target.x(), y: e.target.y() };
}

// ── Testo trasformazione ───────────────────────────────────────────

function onTextTransformEnd(e, id) {
  const node = e.target;
  const idx = texts.value.findIndex(t => t.id === id);
  if (idx === -1) return;
  // Ridimensionamento testo: aggiorniamo fontSize in base alla scala
  const newFontSize = Math.max(8, Math.round((texts.value[idx].fontSize ?? TEXT_SIZE) * node.scaleY()));
  const updated = [...texts.value];
  updated[idx] = {
    ...updated[idx],
    x: node.x(),
    y: node.y(),
    fontSize: newFontSize,
    rotation: node.rotation(),
  };
  texts.value = updated;
  node.scaleX(1);
  node.scaleY(1);
}

function onTextDragEnd(e, id) {
  const idx = texts.value.findIndex(item => item.id === id);
  if (idx === -1) return;
  const updated = [...texts.value];
  updated[idx] = { ...updated[idx], x: e.target.x(), y: e.target.y() };
  texts.value = updated;
}

// ── Transformer ────────────────────────────────────────────────────

function updateTransformer() {
  const tr = transformerRef.value?.getNode();
  if (!tr) return;

  if (selectedNode.value?.type === "image" && imageRef.value) {
    const img = imageRef.value.getNode();
    if (img) { tr.nodes([img]); tr.getLayer()?.batchDraw(); return; }
  }

  if (selectedNode.value?.type === "text") {
    const stage = getStage();
    const textNode = stage?.findOne("#text_" + selectedNode.value.id);
    if (textNode) { tr.nodes([textNode]); tr.getLayer()?.batchDraw(); return; }
  }

  tr.nodes([]);
  tr.getLayer()?.batchDraw();
}

// Aggiorna config transformer in base a selezione
const currentTransformerConfig = computed(() =>
  selectedNode.value?.type === "text" ? textTransformerConfig : imageTransformerConfig
);

watch(tool, () => nextTick(updateTransformer));
watch(imageNode, () => nextTick(updateTransformer));
watch(selectedNode, () => nextTick(updateTransformer));

// ── Export ─────────────────────────────────────────────────────────

function exportImageData() {
  const tr = transformerRef.value?.getNode();
  if (tr) tr.nodes([]);
  const stage = getStage();
  const layer = stage.children?.[0];
  layer?.batchDraw();
  const canvas = stage.toCanvas({ pixelRatio: 1 });
  const imageData = canvas.getContext("2d").getImageData(0, 0, CANVAS_W, CANVAS_H);
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
          <svg v-if="t.id === 'pen'" class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12 20h9" /><path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z" />
          </svg>
          <svg v-else-if="t.id === 'text'" class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round">
            <polyline points="4 7 4 4 20 4 20 7" /><line x1="9" y1="20" x2="15" y2="20" /><line x1="12" y1="4" x2="12" y2="20" />
          </svg>
          <svg v-else-if="t.id === 'image'" class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
            <rect x="3" y="3" width="18" height="18" rx="2" ry="2" /><circle cx="8.5" cy="8.5" r="1.5" /><polyline points="21 15 16 10 5 21" />
          </svg>
          <svg v-else-if="t.id === 'eraser'" class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
            <path d="M20 20H7L3 16a2.12 2.12 0 0 1 0-3L13 3a2.12 2.12 0 0 1 3 0l7 7a2.12 2.12 0 0 1 0 3Z" /><line x1="18" y1="13" x2="13" y2="18" />
          </svg>
          <svg v-else class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round">
            <polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" />
          </svg>
          <span class="tool-label">{{ t.label }}</span>
        </button>
      </div>

      <input ref="fileInputRef" type="file" accept="image/*" hidden @change="onFileSelected" />

      <!-- Barra testo SOPRA il canvas -->
      <div v-if="tool === 'text'" class="text-bar">
        <p class="text-step"><span class="step-num">1</span> Scrivi il testo qui sotto</p>
        <input
          v-model="textDraft"
          class="text-input"
          type="text"
          placeholder="Es: Buon compleanno!"
          maxlength="40"
          enterkeyhint="done"
          @keyup.enter="$event.target.blur()"
          autofocus
        />
        <p class="text-step text-step--secondary"><span class="step-num">2</span> Tocca il canvas per posizionarlo — poi trascinalo o ruotalo</p>
      </div>

      <!-- Canvas -->
      <div class="canvas-card">
        <div ref="containerRef" class="canvas-container">
          <div
            class="canvas-scaler"
            :style="{
              width: CANVAS_W + 'px',
              height: CANVAS_H + 'px',
              transform: `scale(${displayScale})`,
              transformOrigin: 'top left',
            }"
          >
            <v-stage
              ref="stageRef"
              :config="{ width: CANVAS_W, height: CANVAS_H }"
              @mousedown="onPointerDown"
              @mousemove="onPointerMove"
              @mouseup="onPointerUp"
              @touchstart.passive="false"
              @touchstart="onPointerDown"
              @touchmove="onPointerMove"
              @touchend="onPointerUp"
            >
              <v-layer>
                <!-- Sfondo bianco -->
                <v-rect :config="{ x: 0, y: 0, width: CANVAS_W, height: CANVAS_H, fill: WHITE, listening: false }" />

                <!-- Immagine importata -->
                <v-image
                  v-if="imageNode"
                  ref="imageRef"
                  :config="{ ...imageNode, draggable: imageDraggable }"
                  @transformend="onImageTransformEnd"
                  @dragend="onImageDragEnd"
                />

                <!-- Linee disegnate -->
                <v-line v-for="(line, i) in lines" :key="'l' + i" :config="lineConfig(line)" />

                <!-- Testi -->
                <v-text
                  v-for="t in texts"
                  :key="'t' + t.id"
                  :config="textConfig(t)"
                  @dragend="(e) => onTextDragEnd(e, t.id)"
                  @transformend="(e) => onTextTransformEnd(e, t.id)"
                />

                <!-- Transformer unificato -->
                <v-transformer ref="transformerRef" :config="currentTransformerConfig" />
              </v-layer>
            </v-stage>
          </div>
        </div>
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
            <circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="2.5" stroke-dasharray="40" stroke-dashoffset="10" />
          </svg>
          Invio in corso…
        </span>
        <span v-else-if="sendSuccess" class="btn-inner">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><polyline points="20 6 9 17 4 12" /></svg>
          Immagine inviata
        </span>
        <span v-else-if="sendError" class="btn-inner">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round">
            <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
          </svg>
          Errore invio
        </span>
        <span v-else class="btn-inner">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
            <path d="M22 2 L11 13" /><path d="M22 2 L15 22 L11 13 L2 9 Z" />
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
  padding: 16px 16px calc(24px + var(--safe-area-inset-bottom, 0px));
  gap: 16px;
}

/* ── Toolbar ─────────────────────────────────────────────────────── */
.toolbar {
  display: flex;
  gap: 8px;
  overflow-x: auto;
  -webkit-overflow-scrolling: touch;
  padding-bottom: 4px;
  scrollbar-width: none;
}
.toolbar::-webkit-scrollbar { display: none; }

.tool-btn {
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4px;
  min-width: 62px;
  min-height: 62px;
  padding: 8px 10px;
  border: 2px solid rgba(26, 26, 46, 0.18);
  border-radius: 14px;
  background: #ffffff;
  color: #1a1a2e;
  cursor: pointer;
  transition: background 0.15s, transform 0.1s;
  -webkit-tap-highlight-color: transparent;
  touch-action: manipulation;
  user-select: none;
}
.tool-btn:active { transform: scale(0.93); }
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
  font-size: 0.65rem;
  font-weight: 800;          /* bold per e-ink */
  letter-spacing: 0.02em;
  white-space: nowrap;
}

/* ── Canvas card ──────────────────────────────────────────────────── */
.canvas-card {
  background: #ffffff;
  border-radius: 20px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.06), 0 0 0 2px rgba(26,26,46,0.10);
  padding: 16px;
  display: flex;
  justify-content: center;
}

.canvas-container {
  width: 100%;
  padding-top: calc(100% * 122 / 250);
  position: relative;
  overflow: hidden;
  touch-action: none;
  overscroll-behavior: none;
}

.canvas-scaler {
  position: absolute;
  top: 0;
  left: 0;
}
.canvas-scaler :deep(canvas) {
  display: block;
  touch-action: none;
}

/* ── Barra testo ──────────────────────────────────────────────────── */
.text-bar {
  display: flex;
  flex-direction: column;
  gap: 8px;
  background: #ffffff;
  border-radius: 16px;
  padding: 14px 16px;
  box-shadow: 0 0 0 2px rgba(26,26,46,0.10);
}

.text-step {
  margin: 0;
  font-family: "Courier New", monospace;
  font-size: 0.8rem;
  font-weight: 800;
  color: #1a1a2e;
  display: flex;
  align-items: center;
  gap: 8px;
}
.text-step--secondary {
  font-weight: 700;
  color: #555;
  font-size: 0.75rem;
}

.step-num {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  border-radius: 50%;
  background: #1a1a2e;
  color: #f7f4ef;
  font-size: 0.7rem;
  font-weight: 900;
  flex-shrink: 0;
}

.text-input {
  width: 100%;
  padding: 13px 14px;
  border: 2px solid rgba(26, 26, 46, 0.20);
  border-radius: 12px;
  font-size: 16px;
  font-family: "Courier New", monospace;
  font-weight: 700;
  background: #f7f4ef;
  color: #1a1a2e;
  outline: none;
  box-sizing: border-box;
  -webkit-appearance: none;
}
.text-input:focus { border-color: #1a1a2e; }

.spacer { flex: 1; }

/* ── Bottone invio ────────────────────────────────────────────────── */
.send-btn {
  width: 100%;
  padding: 20px 24px;
  border: none;
  border-radius: 16px;
  background: #1a1a2e;
  color: #f7f4ef;
  font-family: "Courier New", monospace;
  font-size: 1.1rem;
  font-weight: 900;          /* massimo bold per e-ink */
  letter-spacing: 0.05em;
  cursor: pointer;
  transition: background 0.25s, transform 0.1s, box-shadow 0.25s;
  box-shadow: 0 4px 16px rgba(26,26,46,0.25);
  -webkit-tap-highlight-color: transparent;
  touch-action: manipulation;
}
.send-btn:not(:disabled):active {
  transform: scale(0.97);
  box-shadow: 0 2px 8px rgba(26,26,46,0.2);
}
.send-btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
  box-shadow: none;
}
.send-btn--success { background: #1e6b45; box-shadow: 0 4px 16px rgba(30,107,69,0.3); }
.send-btn--success:disabled { opacity: 1; }
.send-btn--error   { background: #8b1a1a; box-shadow: 0 4px 16px rgba(139,26,26,0.3); }
.send-btn--error:disabled { opacity: 1; }

.btn-inner {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
}
.btn-inner svg { width: 22px; height: 22px; flex-shrink: 0; }

.spinner { animation: spin 0.9s linear infinite; }
@keyframes spin { to { transform: rotate(360deg); } }

.no-device-hint {
  text-align: center;
  font-family: "Courier New", monospace;
  font-size: 0.85rem;
  font-weight: 700;
  color: #888;
  margin-top: -8px;
}
</style>