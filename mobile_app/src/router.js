import { createRouter, createWebHistory } from "vue-router";

const routes = [
  {
    path: "/",
    name: "Home",
    component: () => import("./views/Home.vue"),
  },
  {
    path: "/parking-disk",
    name: "ParkingDisk",
    component: () => import("./views/ParkingDisk.vue"),
  },
  {
    path: "/draw",
    name: "Drawing",
    component: () => import("./views/Drawing.vue"),
  },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

export default router;
