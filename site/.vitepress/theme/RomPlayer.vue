<script setup>
import { computed, inject, watch } from 'vue'
import { useRoute, withBase } from 'vitepress'
import romVersions from '../rom-versions.json'

const props = defineProps({
  part: { type: Number, required: true }
})
const activePart = inject('active-rom-player')
const route = useRoute()
const opened = computed(() => activePart.value === props.part)
const playerUrl = computed(() => withBase(`/emulator/index.html?part=${props.part}&revision=${romVersions[props.part]}`))
watch(() => route.path, () => { activePart.value = null })
</script>

<template>
  <div class="rom-download-panel rom-player-panel">
    <slot />
    <button v-if="!opened" class="rom-download-button rom-play-button" aria-label="Play in browser" @click="activePart = part">
      Play in browser
    </button>
    <template v-else>
      <p class="rom-player-controls">
        Click <strong>Start Game</strong> in the player to enable sound.
        <strong>Enter</strong>: start / pause.
        <strong>Left / Right</strong>: move.
        <strong>X</strong>: launch.
        Touch controls and gamepads are also supported.
      </p>
      <iframe
        class="rom-player-frame"
        :src="playerUrl"
        :title="`SNES Breakout Part ${part} emulator`"
        allow="autoplay; fullscreen; gamepad; screen-wake-lock"
      ></iframe>
      <button class="rom-player-close" @click="activePart = null">Close emulator</button>
    </template>
  </div>
</template>
