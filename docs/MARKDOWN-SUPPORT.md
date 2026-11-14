# Markdown support

Parsing is md4c with the GitHub dialect plus LaTeX spans, wiki links, and
highlights. `tests/fixtures/showcase.md` has one of everything in this table.

| Structure | Syntax | Status |
| --- | --- | --- |
| Headings | `#` to `######`, and `===` / `---` underlines | Yes |
| Paragraphs, line breaks | blank line, two trailing spaces, `\` | Yes |
| Emphasis | `*em*`, `**strong**`, `***both***` | Yes |
| Strikethrough | `~~text~~` | Yes |
| Highlight | `==text==` | Yes |
| Inline code | `` `code` `` | Yes |
| Code blocks | fenced and indented | Yes, with highlighting |
| Links | inline, reference, autolinks, bare URLs | Yes |
| Heading links | `[x](#heading-id)`, ids as on GitHub | Yes |
| Links to other files | `[x](other.md#section)` | Yes, opens in the reader with back and forward |
| Wiki links | `[[page]]`, `[[page|label]]` | Yes |
| Images | local, web, SVG | Yes; local ones are scaled to fit |
| Lists | bullets, numbers (keeping the start number), nesting | Yes |
| Task lists | `- [ ]`, `- [x]` | Yes |
| Tables | pipes, with column alignment | Yes |
| Block quotes | `>`, nested | Yes |
| Alerts | `> [!NOTE]`, `TIP`, `IMPORTANT`, `WARNING`, `CAUTION` | Yes |
| Footnotes | `[^1]` and `[^1]: text` | Yes, linked both ways |
| Horizontal rule | `---`, `***`, `___` | Yes |
| Front matter | `---` YAML block at the top | Yes, shown as YAML |
| HTML entities | `&copy;`, `&#8212;` | Yes |
| Raw HTML | `<kbd>`, `<sub>`, `<sup>`, `<br>`, `<img>`, `<a name>`, … | The tags Qt's rich text knows; others show their text |
| Math | `$x$`, `$$x$$` | Shown as LaTeX source (typesetting is planned) |
| Diagrams | ` ```mermaid ` | Shown as code |
| `<details>` | collapsible sections | Shown expanded |

## Code highlighting

A small lexer colors comments, strings, numbers, keywords, calls, and type
names. Languages, by family:

- C-like: C, C++, Java, Go, Rust, C#, Kotlin, Swift, Scala, Zig
- Script: JavaScript, TypeScript, JSX, TSX, Dart, PHP, QML
- Hash comments: Python, Ruby, Perl, R, Elixir, Julia, shell (bash, zsh,
  fish), Makefile, Dockerfile, Nix, PowerShell
- Data: JSON, YAML, TOML, INI, CSS, SCSS
- SQL, Lua, Haskell, Elm
- Markup: HTML, XML, SVG, Vue, Svelte
- Diff

A block in any other language is shown plain.
