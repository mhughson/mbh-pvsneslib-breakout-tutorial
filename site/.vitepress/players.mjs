export function romPlayers(markdown) {
  const render = markdown.renderer.rules.html_block
  markdown.renderer.rules.html_block = (tokens, index, ...args) => {
    const html = render ? render(tokens, index, ...args) : tokens[index].content
    return html.replace(
      /<div class="rom-download-panel" data-part="([1-4])">([\s\S]*?)<\/div>/g,
      (_, part, content) => `<RomPlayer :part="${part}">${content}</RomPlayer>`
    )
  }
}
