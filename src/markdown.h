#pragma once

#include <QString>

// Everything the renderer needs to know about how a page should look. The
// backend fills it from the theme, the zoom level, and the open file.
struct MarkdownStyle {
    QString background = "#0e0e10";
    QString foreground = "#c9c9d1";
    QString heading = "#f4f4f6";
    QString muted = "#7f7f8a";
    QString accent = "#FFD60A";
    QString surface = "#1b1b20";
    QString border = "#34343c";
    QString red = "#f7768e";
    QString green = "#9ece6a";
    QString yellow = "#e0af68";
    QString orange = "#ff9e64";
    QString blue = "#7aa2f7";
    QString magenta = "#bb9af7";
    QString cyan = "#7dcfff";
    QString monoFamily = "monospace";
    int fontSize = 16;
    // Local images wider than this are scaled down to fit the page.
    int maxImageWidth = 760;
    // Relative image paths are measured against this directory.
    QString baseDirectory;
};

// Turns Markdown (CommonMark plus the GitHub extensions, footnotes, alerts,
// wiki links, highlights, and LaTeX spans) into the HTML subset that Qt's
// rich text engine lays out. md4c does the parsing, so structure follows the
// CommonMark spec exactly; this only decides how each element looks.
QString renderMarkdown(const QString &markdown, const MarkdownStyle &style);
