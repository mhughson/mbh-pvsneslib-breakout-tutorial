const games = {
  '1': '../roms/fakeout-part1.sfc',
  '2': '../roms/fakeout-part2.sfc',
  '3': '../roms/fakeout-part3.sfc',
  '4': '../roms/fakeout-part4.sfc'
}
const part = new URLSearchParams(location.search).get('part')
const revision = new URLSearchParams(location.search).get('revision')
const status = document.getElementById('status')
let failed = false

function fail(message) {
  failed = true
  console.error(message)
  status.hidden = false
  status.className = 'error'
  status.setAttribute('role', 'alert')
  status.textContent = `${message} Close the player and try again, or download the ROM instead.`
  document.getElementById('game').hidden = true
}

window.addEventListener('error', event => {
  fail(event.message || 'An emulator file could not be loaded.')
}, true)
window.addEventListener('unhandledrejection', event => {
  // The pinned loader's optional localhost update check has no rejection handler.
  if (event.reason?.stack?.includes('checkForUpdates')) {
    console.warn('EmulatorJS update check unavailable; using the verified local runtime.', event.reason)
    event.preventDefault()
    return
  }
  if (event.reason?.name === 'NotAllowedError' && /wake\s*lock/i.test(event.reason.message)) {
    console.warn('Screen wake lock unavailable; the emulator remains playable.', event.reason)
    event.preventDefault()
    return
  }
  fail(`The emulator could not start: ${event.reason?.message || event.reason}`)
})

if (!Object.hasOwn(games, part) || (revision !== null && !/^[a-f0-9]{64}$/.test(revision))) {
  fail('Unknown tutorial ROM.')
} else {
  window.EJS_player = '#game'
  window.EJS_core = 'snes'
  const gameUrl = new URL(games[part], location.href)
  if (revision) gameUrl.searchParams.set('v', revision)
  window.EJS_gameUrl = gameUrl.href
  window.EJS_pathtodata = new URL('data/', location.href).href
  window.EJS_gameName = `SNES Breakout Part ${part}`
  window.EJS_gameID = Number(part)
  window.EJS_threads = false
  window.EJS_startOnLoaded = false
  window.EJS_language = 'en-US'
  // The 4.2.3 loader disables locale detection when this flag is false.
  window.EJS_disableAutoLang = false
  window.EJS_color = '#256ea7'
  window.EJS_backgroundColor = '#111'
  window.EJS_alignStartButton = 'center'
  window.EJS_Buttons = {
    cheat: false,
    netplay: false,
    screenRecord: false
  }
  window.EJS_ready = () => {
    if (!failed) status.textContent = 'Click Start Game to begin. The first launch downloads the SNES core.'
  }
  window.EJS_onGameStart = () => { if (!failed) status.hidden = true }
  const script = document.createElement('script')
  script.src = 'data/loader.js'
  document.body.appendChild(script)
}
