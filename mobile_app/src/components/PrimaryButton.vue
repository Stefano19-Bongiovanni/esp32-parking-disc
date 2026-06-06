<script setup>
const props = defineProps({
  label: { type: String, required: true }
});
const emit = defineEmits(['click']);

const handleClick = (event) => {
  emit('click', event);
};
</script>

<template>
  <button class="primary" type="button" @click="handleClick">
    {{ label }}
  </button>
</template>

<style scoped>
.primary {
  flex: 1;
  min-width: 0;
  max-width: 240px;
  width: 100%;
  border: none;
  border-radius: 999px;
  padding: 14px 18px;
  background: var(--color-button);
  color: var(--color-button-text);
  box-shadow: var(--color-shadow);
  cursor: pointer;
  transition: transform 0.2s ease, filter 0.2s ease;
}

.primary:hover {
  transform: translateY(-1px);
  filter: brightness(1.05);
}

.primary:active {
  transform: translateY(0);
}
</style>
