<script setup>
import HeaderBar from "@/components/HeaderBar.vue";
</script>

<template>
  <main class="page-shell">
    <HeaderBar title="Disco orario" :showBackButton="true" />
    <section class="screen">
      <div class="placeholder">
        Contenuto del disco orario in fase di sviluppo.
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

.placeholder {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-muted);
  font-size: 1rem;
  background: rgba(108, 58, 92, 0.06);
  border-radius: 18px;
  padding: 24px;
}
</style>
