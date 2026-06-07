<script setup>
import { useRouter } from "vue-router";

const props = defineProps({
  title: { type: String, default: "" },
  showBackButton: { type: Boolean, default: false },
});

const router = useRouter();

const goBack = () => {
  if (window.history.length > 1) {
    router.back();
  } else {
    router.push("/");
  }
};
</script>

<template>
  <header class="header-row">
    <button
      v-if="showBackButton"
      class="back-button"
      type="button"
      @click="goBack"
      aria-label="Indietro"
    >
      <span class="mdi mdi-chevron-left" aria-hidden="true"></span>
      <span class="back-text">Home</span>
    </button>
    <div class="title">{{ title }}</div>
  </header>
</template>

<style scoped>
.header-row {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--color-surface);
  border-bottom: 1px solid var(--color-border);
  padding: 16px;
  position: sticky;
  top: 0;
  z-index: 10;
}

.back-button {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%);
  background: transparent;
  border: none;
  color: var(--color-primary);
  font-size: 1.2rem;
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  padding: 10px 14px;
  gap: 6px;
}

.back-button .mdi {
  font-size: 1.5rem;
}

.back-button .back-text {
  font-weight: 600;
  letter-spacing: 0.02em;
}

.title {
  font-weight: 700;
  font-size: 1.15rem;
  letter-spacing: 0.02em;
  color: var(--color-text);
}
</style>
