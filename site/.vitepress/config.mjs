import { defineConfig } from 'vitepress'
import { execFile } from 'node:child_process'
import { resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { comparisons } from './comparisons.mjs'
import { romPlayers } from './players.mjs'

export const base = '/mbh-pvsneslib-breakout-tutorial/'
export const pages = new Map([
  ['README.md', 'index.md'],
  ['README-part2.md', 'part-2.md'],
  ['README-part3.md', 'part-3.md'],
  ['README-part4.md', 'part-4.md'],
  ['README-whats-next.md', 'whats-next.md'],
  ['CONTRIBUTING.md', 'contributing.md']
])

function liveTutorialPages() {
  return {
    name: 'live-tutorial-pages',
    configureServer(server) {
      const root = fileURLToPath(new URL('../../', import.meta.url))
      const key = file => process.platform === 'win32' ? resolve(file).toLowerCase() : resolve(file)
      const sources = [...pages.keys()].map(file => resolve(root, file))
      const watched = new Set(sources.map(key))
      let timer
      let running = Promise.resolve()
      server.watcher.add(sources)
      const update = file => {
        if (!watched.has(key(file))) return
        clearTimeout(timer)
        timer = setTimeout(() => {
          running = running.then(() => new Promise(done => {
            execFile(process.execPath, [resolve(root, 'site', 'prepare.mjs')], { cwd: root },
              (error, stdout, stderr) => {
                if (error) {
                  const message = `Tutorial page refresh failed:\n${stderr || error.message}`
                  server.config.logger.error(message)
                  server.ws.send({ type: 'error', err: { message, stack: error.stack } })
                } else {
                  server.config.logger.info('Refreshed live tutorial pages.')
                }
                done()
              })
          }))
        }, 150)
      }
      server.watcher.on('change', update)
      server.httpServer?.once('close', () => {
        clearTimeout(timer)
        server.watcher.off('change', update)
      })
    }
  }
}

export default defineConfig({
  title: 'SNES Breakout Tutorial',
  description: 'A step-by-step Breakout tutorial with PVSnesLib.',
  base,
  srcDir: 'content',
  outDir: '../build/site',
  cacheDir: '.vitepress/cache',
  cleanUrls: false,
  lastUpdated: true,
  transformPageData(pageData) {
    if (pageData.relativePath === 'index.md') {
      pageData.title = 'Part 1: The Core Mechanics'
    }
  },
  themeConfig: {
    nav: [
      { text: 'Tutorial', link: '/' },
      { text: 'Contributing', link: '/contributing' }
    ],
    sidebar: [
      { text: 'Part 1 · The Core Mechanics', link: '/' },
      { text: 'Part 2 · Graphics and sound', link: '/part-2' },
      { text: 'Part 3 · Screens and effects', link: '/part-3' },
      { text: 'Part 4 · Levels and power-ups', link: '/part-4' },
      { text: "What's Next", link: '/whats-next' }
    ],
    outline: { level: 2, label: 'On this page' },
    search: { provider: 'local' },
    socialLinks: [{ icon: 'github', link: 'https://github.com/mhughson/mbh-pvsneslib-breakout-tutorial' }],
    editLink: {
      pattern: 'https://github.com/mhughson/mbh-pvsneslib-breakout-tutorial/issues',
      text: 'Suggest an improvement'
    }
  },
  markdown: {
    anchor: {
      slugify: title => title.toLowerCase().replace(/[^\w -]/g, '').replace(/ /g, '-')
    },
    config: markdown => {
      comparisons(markdown, base)
      romPlayers(markdown)
    }
  },
  vite: {
    publicDir: '../public',
    plugins: [liveTutorialPages()]
  }
})
