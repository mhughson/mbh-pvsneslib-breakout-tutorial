import { cp, mkdir, readFile } from 'node:fs/promises'
import { createHash } from 'node:crypto'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { base, pages } from './.vitepress/config.mjs'
import { prepareEmulator, writeIfChanged } from './emulator-assets.mjs'

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const repository = 'https://github.com/mhughson/mbh-pvsneslib-breakout-tutorial'
const content = resolve(root, 'site/content')
const assets = resolve(root, 'site/public')
await mkdir(content, { recursive: true })
await mkdir(assets, { recursive: true })

function siteLink(link) {
  if (/^(?:[a-z]+:|#|\/\/)/i.test(link)) return link
  const [path, ...fragment] = link.split('#')
  const anchor = fragment.length ? `#${fragment.join('#')}` : ''
  if (pages.has(path)) return pages.get(path) + anchor
  if (/\.(?:png|mp4|sfc)$/i.test(path)) return `/${path}${anchor}`
  return `${repository}/blob/main/${path}${anchor}`
}

for (const [source, target] of pages) {
  const markdown = await readFile(resolve(root, source), 'utf8')
  // Leave code examples unchanged; transform only prose links.
  const parts = markdown.split(/(```[\s\S]*?```)/g)
  const transformed = parts.map((part, index) => index % 2 ? part : part
    .replace(/(!?\[[^\]]*\])\(([^)\n]+)\)/g, (_, label, link) => `${label}(${siteLink(link)})`)
    .replace(/^(\[[^\]]+\]:\s+)(\S+)/gm, (_, label, link) => label + siteLink(link))
    .replace(/((?:src|href|poster)=["'])([^"']+)(["'])/g, (_, start, link, end) => {
      const target = siteLink(link)
      return start + (start.startsWith('href')
        ? target.replace(/^\/(?!\/)/, base)
        : target) + end
    })
  ).join('')
  await writeIfChanged(resolve(content, target), Buffer.from(transformed))
}

for (const path of ['screenshots', 'videos']) {
  await cp(resolve(root, path), resolve(assets, path), { recursive: true })
}
for (const path of ['sprites.png', 'breakout.sfc']) {
  await writeIfChanged(resolve(assets, path), await readFile(resolve(root, path)))
}
for (const part of ['part2', 'part3', 'part4']) {
  await mkdir(resolve(assets, part), { recursive: true })
  for (const file of ['breakout.sfc', 'backdrop.png', 'ball.png', 'paddle.png', 'playfield.png']) {
    await writeIfChanged(resolve(assets, part, file), await readFile(resolve(root, part, file)))
  }
}
for (const [part, file] of [['part3', 'title-logo.png'], ['part4', 'title-logo.png'], ['part4', 'capsule.png']]) {
  await writeIfChanged(resolve(assets, part, file), await readFile(resolve(root, part, file)))
}
// EmulatorJS caches ROMs by basename and size, not by their full URLs.
await mkdir(resolve(assets, 'roms'), { recursive: true })
const romVersions = {}
for (const part of [1, 2, 3, 4]) {
  const source = part === 1 ? resolve(root, 'breakout.sfc') : resolve(root, `part${part}`, 'breakout.sfc')
  const data = await readFile(source)
  await writeIfChanged(resolve(assets, 'roms', `fakeout-part${part}.sfc`), data)
  romVersions[part] = createHash('sha256').update(data).digest('hex')
}
await writeIfChanged(resolve(root, 'site', '.vitepress', 'rom-versions.json'), Buffer.from(JSON.stringify(romVersions)))
console.log('Prepared tutorial pages and public assets from repository sources.')
await prepareEmulator(root, assets)
