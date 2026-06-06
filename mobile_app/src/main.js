import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import { createPinia } from "pinia";
import VueKonva from "vue-konva";

import router from "@/router";

const app = createApp(App);
const pinia = createPinia();
app.use(VueKonva);
app.use(pinia);
app.use(router);



router.isReady().then(() => {
  app.mount("#app");
})