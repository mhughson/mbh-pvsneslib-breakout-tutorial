import assert from 'node:assert/strict'
import test from 'node:test'
import { createMarkdownRenderer } from 'vitepress'
import { comparisonSides } from '../.vitepress/comparisons.mjs'
import config from '../.vitepress/config.mjs'

test('comparison separates removed and added C lines, keeping context', () => {
  const code = 'if (ready)\n{\n    start_old(); // [!code --]\n    start_new(); // [!code ++]\n}\n'
  assert.deepEqual(comparisonSides(code), {
    before: 'if (ready)\n{\n    start_old(); // [!code --]\n}\n',
    after: 'if (ready)\n{\n    start_new(); // [!code ++]\n}\n'
  })
})

test('unequal changes keep following context aligned', () => {
  const sides = comparisonSides('before();\nold(); // [!code --]\nnew(); // [!code ++]\nextra(); // [!code ++]\nafter();\n')
  assert.equal(sides.before.split('\n').indexOf('after();'), 3)
  assert.equal(sides.after.split('\n').indexOf('after();'), 3)
  assert.equal(sides.before.split('\n')[1], 'old(); // [!code --]')
  assert.equal(sides.after.split('\n')[2], 'extra(); // [!code ++]')
})

test('Makefile comments use the same comparison presentation', () => {
  assert.equal(comparisonSides('all: old # [!code --]\nall: new # [!code ++]\n').after,
    'all: new # [!code ++]\n')
  assert.equal(comparisonSides('int unchanged;\n'), null)
})

test('rendered comparisons preserve highlighting and hide annotations and alignment rows from copying', async () => {
  const markdown = await createMarkdownRenderer(process.cwd(), config.markdown)
  const html = markdown.render('## 1. Change velocity\n\n[Jump](#1-change-velocity)\n\n[Download ROM](/part4/breakout.sfc)\n\n```c\nint old; // [!code --]\nint level; // [!code ++]\nint lives; // [!code ++]\n```\n')
  const before = html.split('aria-label="Before code"')[1].split('</section>')[0]
  const after = html.split('aria-label="After code"')[1].split('</section>')[0]
  assert.match(before, /diff remove/)
  assert.match(after, /diff add/)
  assert.match(before, /comparison-spacer vp-copy-ignore/)
  assert.match(html, /--shiki-dark/)
  assert.doesNotMatch(html, /\[!code|__TUTORIAL_COMPARISON_SPACER__/)
  assert.doesNotMatch(after, />old[<;]/)
  assert.match(html, /id="1-change-velocity"/)
  assert.match(html, /href="#1-change-velocity"/)
  assert.match(html, /href="\/mbh-pvsneslib-breakout-tutorial\/part4\/breakout\.sfc" download/)
})

test('ROM panels enhance portable download markup without eagerly rendering an iframe', async () => {
  const markdown = await createMarkdownRenderer(process.cwd(), config.markdown)
  const panel = '<div class="rom-download-panel" data-part="4">\n<a href="/part4/breakout.sfc" download>Download ROM</a>\n</div>'
  const html = markdown.render(panel)
  assert.match(html, /<RomPlayer :part="4">/)
  assert.match(html, /href="\/part4\/breakout.sfc" download/)
  assert.doesNotMatch(html, /<iframe/)
  assert.doesNotMatch(markdown.render(`\`\`\`html\n${panel}\n\`\`\``), /<RomPlayer/)
})
