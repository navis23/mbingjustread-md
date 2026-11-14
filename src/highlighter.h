#pragma once

#include <QString>

struct MarkdownStyle;

// Colors the code of a fenced block and returns it as escaped HTML. It is a
// small lexer, not a grammar: comments, strings, numbers, keywords, and
// calls, which is what makes code readable at a glance. An unknown language
// comes back as plain escaped text.
QString highlightCode(const QString &code, const QString &language, const MarkdownStyle &style);
