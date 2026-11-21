import DefaultTheme from 'vitepress/theme'
import { ref } from 'vue'
import RomPlayer from './RomPlayer.vue'
import './style.css'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.provide('active-rom-player', ref(null))
    app.component('RomPlayer', RomPlayer)
  }
}
