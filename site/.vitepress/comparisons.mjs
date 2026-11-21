const annotation = /\s*(?:\/\/|#|;)\s*\[!code (\+\+|--)\]\s*$/
const spacer = '/* __TUTORIAL_COMPARISON_SPACER__ */'

export function comparisonSides(code) {
  const before = []
  const after = []
  const removed = []
  const added = []
  let changed = false
  const flushChange = () => {
    const length = Math.max(removed.length, added.length)
    for (let index = 0; index < length; index++) {
      before.push(removed[index] ?? spacer)
      after.push(added[index] ?? spacer)
    }
    removed.length = 0
    added.length = 0
  }
  for (const line of code.trimEnd().split('\n')) {
    const mark = line.match(annotation)?.[1]
    changed ||= Boolean(mark)
    if (mark === '--') removed.push(line)
    else if (mark === '++') added.push(line)
    else {
      flushChange()
      before.push(line)
      after.push(line)
    }
  }
  flushChange()
  return changed ? { before: before.join('\n') + '\n', after: after.join('\n') + '\n' } : null
}

export function comparisons(markdown, base = '/') {
  const renderLink = markdown.renderer.rules.link_open
  markdown.renderer.rules.link_open = (tokens, index, options, env, renderer) => {
    const token = tokens[index]
    const href = token.attrGet('href')
    if (href?.startsWith('/') && /\.sfc(?:[?#]|$)/i.test(href)) {
      token.attrSet('href', base + href.slice(1))
      token.attrSet('download', '')
    }
    return renderLink(tokens, index, options, env, renderer)
  }
  const renderFence = markdown.renderer.rules.fence
  markdown.renderer.rules.fence = (tokens, index, options, env, renderer) => {
    const token = tokens[index]
    const sides = comparisonSides(token.content)
    if (!sides) return renderFence(tokens, index, options, env, renderer)
    const pane = (name, code) => {
      const clone = Object.assign(Object.create(Object.getPrototypeOf(token)), token, { content: code })
      const copied = [...tokens]
      copied[index] = clone
      const rendered = renderFence(copied, index, options, env, renderer)
        .replace(/<span class="line[^"]*">[^\n]*__TUTORIAL_COMPARISON_SPACER__[^\n]*<\/span>(\n|(?=<\/code>))/g,
          '<span class="line comparison-spacer vp-copy-ignore" aria-hidden="true"> </span><span class="vp-copy-ignore">\n</span>')
      return `<section class="comparison-pane comparison-${name.toLowerCase()}" aria-label="${name} code"><div class="comparison-label">${name}</div>${rendered}</section>`
    }
    return `<div class="code-comparison" aria-label="Code change comparison">${pane('Before', sides.before)}${pane('After', sides.after)}</div>`
  }
}
