import assert from 'node:assert/strict'
import { mkdtemp, readFile, readdir, rm, stat, utimes } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { runInNewContext } from 'node:vm'
import test from 'node:test'
import { writeIfChanged } from '../emulator-assets.mjs'

const source = await readFile(new URL('../emulator/player.mjs', import.meta.url), 'utf8')

test('refreshing unchanged assets preserves their timestamps and avoids hot reload', async t => {
  const directory = await mkdtemp(join(tmpdir(), 'snes-emulator-assets-'))
  t.after(() => rm(directory, { recursive: true, force: true }))
  const path = join(directory, 'loader.js')
  const data = Buffer.from('verified loader')
  await writeIfChanged(path, data)
  await utimes(path, new Date('2000-01-01'), new Date('2000-01-01'))
  const before = await stat(path)
  await writeIfChanged(path, data)
  assert.equal((await stat(path)).mtimeMs, before.mtimeMs)
})

test('concurrent asset preparation leaves complete files and no temporary files', async t => {
  const directory = await mkdtemp(join(tmpdir(), 'snes-emulator-assets-'))
  t.after(() => rm(directory, { recursive: true, force: true }))
  const path = join(directory, 'core.data')
  const data = Buffer.alloc(256 * 1024, 42)
  await Promise.all([1, 2, 3, 4].map(() => writeIfChanged(path, data)))
  assert.deepEqual(await readFile(path), data)
  assert.deepEqual(await readdir(directory), ['core.data'])
})

function boot(search) {
  const scripts = []
  const errors = []
  const warnings = []
  const status = { setAttribute(name, value) { this[name] = value } }
  const game = {}
  const handlers = {}
  const window = { addEventListener: (event, handler) => { handlers[event] = handler } }
  runInNewContext(source, {
    window, URL, URLSearchParams,
    location: { search, href: `https://example.com/tutorial/emulator/index.html${search}` },
    console: { error: message => errors.push(message), warn: message => warnings.push(message) },
    document: {
      getElementById: id => id === 'status' ? status : game,
      createElement: () => ({}),
      body: { appendChild: script => scripts.push(script) }
    }
  })
  return { window, scripts, errors, warnings, status, game, handlers }
}

test('iframe bootstrap resolves a ROM and runtime within a nested Pages base', () => {
  const result = boot('?part=4')
  assert.equal(result.window.EJS_gameUrl, 'https://example.com/tutorial/roms/fakeout-part4.sfc')
  assert.equal(result.window.EJS_pathtodata, 'https://example.com/tutorial/emulator/data/')
  assert.equal(result.scripts.length, 1)
  assert.deepEqual(result.errors, [])
})

test('chapter ROMs have distinct basenames to prevent same-size ROM cache collisions', () => {
  const names = [1, 2, 3, 4].map(part =>
    new URL(boot(`?part=${part}`).window.EJS_gameUrl).pathname.split('/').pop()
  )
  assert.equal(new Set(names).size, 4)
})

test('ROM revisions refresh cached artwork without changing save-file basenames', () => {
  const revision = 'a'.repeat(64)
  const result = boot(`?part=3&revision=${revision}`)
  assert.equal(result.window.EJS_gameUrl,
    `https://example.com/tutorial/roms/fakeout-part3.sfc?v=${revision}`)
  assert.equal(boot('?part=3&revision=invalid').scripts.length, 0)
})

test('unknown ROM selections visibly fail without loading the emulator', () => {
  for (const search of ['', '?part=__proto__', '?part=https://example.org/game.sfc']) {
    const result = boot(search)
    assert.equal(result.scripts.length, 0)
    assert.equal(result.status.role, 'alert')
    assert.match(result.status.textContent, /Unknown tutorial ROM.*download the ROM instead/)
    assert.equal(result.game.hidden, true)
    assert.equal(result.errors.length, 1)
  }
})

test('optional update and wake-lock failures do not hide the self-hosted game', () => {
  for (const reason of [
    { stack: 'TypeError: Failed to fetch\n at EmulatorJS.checkForUpdates' },
    { name: 'NotAllowedError', message: 'Wake Lock permission request denied' },
    { name: 'NotAllowedError', message: "Failed to execute 'request' on 'WakeLock': The requesting page is not visible" }
  ]) {
    const result = boot('?part=4')
    let handled = false
    result.handlers.unhandledrejection({ reason, preventDefault: () => { handled = true } })
    assert.equal(handled, true)
    assert.equal(result.warnings.length, 1)
    assert.deepEqual(result.errors, [])
    assert.notEqual(result.game.hidden, true)
  }
})

test('a fatal startup error remains visible after loader callbacks', () => {
  const result = boot('?part=4')
  result.handlers.unhandledrejection({ reason: { message: 'WebAssembly failed' } })
  result.window.EJS_ready()
  result.window.EJS_onGameStart()
  assert.equal(result.status.role, 'alert')
  assert.notEqual(result.status.hidden, true)
  assert.match(result.status.textContent, /WebAssembly failed/)
})
