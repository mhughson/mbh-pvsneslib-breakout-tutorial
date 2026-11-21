import { createHash, randomUUID } from 'node:crypto'
import { mkdir, readFile, rename, rm, writeFile } from 'node:fs/promises'
import { dirname, resolve } from 'node:path'

const version = '4.2.3'
const cdn = `https://cdn.emulatorjs.org/${version}/data/`
const files = [
  ['loader.js', '69e0903bf1e2f62ced78895e7e511fa26e11316f7eb734925c35e919ba1287b2'],
  ['emulator.min.js', '6aec3fd7bb2721255801b0a6af02e47e78b05e28a1822b1f213aacbd348abaee'],
  ['emulator.min.css', '16406c60b2dc3b04ae9b115e308613e6f567a0cc7068e21d9d0c1e5030fb395e'],
  ['compression/extract7z.js', '4ac9933b995a516cb6b3ca4027db860278372a20c21c405d93ceb1998498853a'],
  ['cores/snes9x-wasm.data', 'eaa0bcfce67673809886e50387a80a616b719502175db64c090d04c9d75958ee'],
  ['cores/snes9x-legacy-wasm.data', '7d427a575cefad98ff400493fa1d7e892da63fe7bab68979babd9cea0bfaaf3b'],
  ['cores/reports/snes9x.json', 'dc7ac963eb7935a7ac78956235ac0b8912ec785c57026336825aa2ed8031b3ad'],
  ['LICENSE', 'a617fab6d251b3302b1cb2c1c94f52d1c584f6fcb2fc218ec1a475bc08053a61',
    `https://raw.githubusercontent.com/EmulatorJS/EmulatorJS/v${version}/LICENSE`],
  ['SNES9X-LICENSE', '70efeee282d82a6e9d26aeed5466d08c632369858371dc6a4644c8dcedc2be78',
    'https://raw.githubusercontent.com/libretro/snes9x/49f484569ff2aec7ff08e7598a97d6c9e6eae72d/LICENSE']
]

export async function writeIfChanged(path, data) {
  try {
    if ((await readFile(path)).equals(data)) return
  } catch (error) {
    if (error.code !== 'ENOENT') throw error
  }
  await mkdir(dirname(path), { recursive: true })
  const temporary = `${path}.${randomUUID()}.tmp`
  try {
    await writeFile(temporary, data)
    try {
      await rename(temporary, path)
    } catch (error) {
      if (!['EPERM', 'EEXIST'].includes(error.code) || !(await readFile(path)).equals(data)) throw error
      console.warn(`Another preparation installed identical data at ${path}; reusing it.`)
    }
  } finally {
    await rm(temporary, { force: true })
  }
}

export async function prepareEmulator(root, publicDir) {
  const cache = resolve(root, 'build', 'emulator-cache', version)
  await Promise.all(files.map(async ([file, expectedHash, source]) => {
    const path = resolve(cache, ...file.split('/'))
    let data
    try {
      data = await readFile(path)
    } catch (error) {
      if (error.code !== 'ENOENT') throw error
      const response = await fetch(source || cdn + file)
      if (!response.ok) throw new Error(`Could not download EmulatorJS ${file}: HTTP ${response.status}`)
      data = Buffer.from(await response.arrayBuffer())
    }
    if (createHash('sha256').update(data).digest('hex') !== expectedHash) {
      throw new Error(`EmulatorJS checksum mismatch: ${file}. Remove that cached file and retry.`)
    }
    await writeIfChanged(path, data)
    const target = resolve(publicDir, 'emulator', 'data', ...file.split('/'))
    await writeIfChanged(target, data)
  }))
  for (const file of ['index.html', 'player.mjs']) {
    const data = await readFile(resolve(root, 'site', 'emulator', file))
    await writeIfChanged(resolve(publicDir, 'emulator', file), data)
  }
}
