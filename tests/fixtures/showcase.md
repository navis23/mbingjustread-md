---
title: Markdown showcase
tags: [reader, test]
---

# Markdown showcase

A page that uses **every structure** the reader supports, so one look shows
whether it renders *accurately*. It has ~~mistakes~~ strikethrough, `inline code`,
==highlights==, a [link](https://commonmark.org), an autolink to
https://github.com, and a footnote[^spec].

## Lists

1. Ordered lists keep their numbers
2. And nest
   - bullets inside
   - with `code`
3. As deep as needed

- [x] Parse with md4c
- [x] Follow the Omarchy theme
- [ ] Typeset math

## Code

```cpp
// Opens the file and renders it.
int main(int argc, char *argv[]) {
    Backend backend(&theme);
    backend.open("README.md"); /* that is all */
    return app.exec() + 42;
}
```

```python
def slug(title: str) -> str:
    """The id GitHub gives a heading."""
    return "-".join(title.lower().split())  # close enough
```

```diff
- old line
+ new line
```

## Table

| Element   | Syntax        | Supported |
| :-------- | :-----------: | --------: |
| Heading   | `# Title`     |       yes |
| Table     | pipes         |       yes |
| Footnote  | `[^1]`        |       yes |

## Quotes and alerts

> Reading is the whole job.
>
> > Nested quotes work too.

> [!NOTE]
> Alerts use the GitHub syntax.

> [!WARNING]
> This one is a warning.

---

### Smaller headings

#### Level four

Math stays readable as source: $e^{i\pi} + 1 = 0$. Raw HTML works for the
tags Qt knows, like <kbd>Ctrl</kbd> + <kbd>C</kbd> and H<sub>2</sub>O.

Jump back to [the lists](#lists).

[^spec]: Parsing follows the CommonMark specification.
