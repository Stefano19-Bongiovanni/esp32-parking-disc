const stageConfig = computed(() => ({
width: CANVAS_W,
height: CANVAS_H,
}));

// Configurazione Transformer dinamica per supportare stretch e rotazione
const transformerConfig = computed(() => {
const isText = selectedNodeId.value.startsWith("text_");
return {
rotateEnabled: true,
keepRatio: isText, // Il testo non si deforma, l'immagine sì (stretch libero)
enabledAnchors: isText
? ["top-left", "top-right", "bottom-left", "bottom-right"]
: ["top-left", "top-right", "bottom-left", "bottom-right", "top-center", "bottom-center", "left-center", "right-center"],
anchorSize: 16,
anchorCornerRadius: 8,
anchorFill: WHITE,
anchorStroke: "#1a1a2e",
borderStroke: "#1a1a2e",
borderStrokeWidth: 2,
borderDash: [4, 4],
padding: 4,
};
});

const tools = [

function lineConfig(line) {
const isEraser = line.tool === "eraser";
return {
points: line.points,
stroke: isEraser ? WHITE : BLACK,
strokeWidth: isEraser ? ERASER_WIDTH : PEN_WIDTH,
tension: 0.5,
lineCap: "round",
lineJoin: "round",
globalCompositeOperation: "source-over",
listening: false, // Le linee non catturano eventi click
};
}

function textConfig(t) {
const isSelected = selectedNodeId.value === "text_" + t.id && tool.value === "text";
return {
id: "text_" + t.id,
name: "textNode",
x: t.x,
y: t.y,
scaleX: t.scaleX || 1,
scaleY: t.scaleY || 1,
rotation: t.rotation || 0,
text: t.text || "...", // Placeholder visivo se vuoto
fontSize: TEXT_SIZE,
fontFamily: FONT_FAMILY,

function onPointerDown(e) {
if (showClearConfirm.value) showClearConfirm.value = false;

const clickedOnEmpty = e.target === getStage() || e.target.name() === "background";

if (tool.value === "text") {
// Se tocca uno spazio vuoto: CREA nuovo testo e selezionalo
if (clickedOnEmpty) {
const pos = getPointerPos(e);
if (!pos) return;
const newId = ++textIdCounter;
texts.value.push({
id: newId,
x: Math.min(pos.x, CANVAS_W - 20),
y: Math.min(pos.y, CANVAS_H - TEXT_SIZE),
text: "Testo",
scaleX: 1,
scaleY: 1,
rotation: 0
});
selectedNodeId.value = "text_" + newId;
nextTick(updateTransformer);
return;
}
// Se tocca un testo esistente: selezionalo
if (e.target.name() === "textNode") {
selectedNodeId.value = e.target.id();
nextTick(updateTransformer);
return;
}
return;
}

if (tool.value === "image") {

async function onFileSelected(e) {
const file = e.target.files?.[0];
e.target.value = "";
if (!file) return;

try {
const { image, width, height, x, y } = await loadBwImage(file);
imageNode.value = {
id: "img",
name: "imageNode",
image,
x,
y,
width,
height,
scaleX: 1,
scaleY: 1,
rotation: 0,
};
tool.value = "image";
selectedNodeId.value = "img";
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
scaleX: node.scaleX(),
scaleY: node.scaleY(),
rotation: node.rotation(),
};
}

function onImageDragEnd(e) {
if (!imageNode.value) return;
imageNode.value.x = e.target.x();
imageNode.value.y = e.target.y();
}

function onTextTransformEnd(e, id) {
const t = texts.value.find((item) => item.id === id);
if (t) {
const node = e.target;
t.x = node.x();
t.y = node.y();
t.scaleX = node.scaleX();
t.scaleY = node.scaleY();
t.rotation = node.rotation();
}
}

function onTextDragEnd(e, id) {
const t = texts.value.find((item) => item.id === id);
if (t) {
t.x = e.target.x();
t.y = e.target.y();
}
}

// Gestione corretta e affidabile del Transformer per Immagini e Testi
function updateTransformer() {
const tr = transformerRef.value?.getNode();
const stage = getStage();
if (!tr || !stage) return;

let selectedNode = null;
if (tool.value === "image" && selectedNodeId.value === "img") {
selectedNode = stage.findOne("#img");
} else if (tool.value === "text" && selectedNodeId.value.startsWith("text_")) {
selectedNode = stage.findOne("#" + selectedNodeId.value);
}

if (selectedNode) {
tr.nodes([selectedNode]);
} else {
tr.nodes([]);
}

tr.getLayer()?.batchDraw();
}

// Reactivity per il testo in fase di editing

            <v-line
              v-for="(line, i) in lines"
              :key="'l' + i"
              :config="lineConfig(line)"
            />

            <v-text
              v-for="t in texts"
              :key="'t' + t.id"
              :config="textConfig(t)"
              @transformend="(e) => onTextTransformEnd(e, t.id)"
              @dragend="(e) => onTextDragEnd(e, t.id)"
            />

            <!-- Transformer per resize immagine e testo -->
            <v-transformer ref="transformerRef" :config="transformerConfig" />
          </v-layer>
        </v-stage>
      </div>
    </div>
  </div>

  <!-- UX MIGLIORATA: Barra di Testo Contestuale -->
  <div v-if="tool === 'text'" class="text-bar-container">
    <div v-if="selectedNodeId.startsWith('text_')" class="text-bar">
      <input
        v-model="activeTextContent"
        class="text-input"
        type="text"
        placeholder="Scrivi qui..."
        enterkeyhint="done"
        autofocus
        @keyup.enter="$event.target.blur()"
      />
      <p class="text-hint">Usa le maniglie per ruotare o ingrandire. Trascina per spostare.</p>
    </div>
    <div v-else class="text-bar text-bar-empty">
      <p class="text-hint-primary">👆 <strong>Tocca il canvas</strong> nel punto in cui vuoi inserire il testo.</p>
    </div>
  </div>
  
  <!-- UX Immagine: hint -->
  <div v-if="tool === 'image' && imageNode" class="text-bar text-bar-empty">
     <p class="text-hint-primary">Usa le maniglie per stretch, ingrandire e ruotare.</p>
     <button class="btn-replace-img" @click="fileInputRef.click()">Cambia immagine</button>
  </div>

  <div class="spacer" />
