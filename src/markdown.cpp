#include "markdown.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QStringList>
#include <QTextDocumentFragment>
#include <QUrl>

#include <md4c.h>

#include "highlighter.h"

namespace {

QString escape(const QString &text) { return text.toHtmlEscaped(); }

// md4c hands attributes (link targets, titles, code languages) as pieces so
// that entities and escapes arrive already separated; join them as plain text.
QString attribute(const MD_ATTRIBUTE &a) {
    QString result;
    for (int i = 0; a.substr_offsets[i] < a.size; ++i) {
        const MD_OFFSET start = a.substr_offsets[i];
        const QString part = QString::fromUtf8(a.text + start, a.substr_offsets[i + 1] - start);
        if (a.substr_types[i] == MD_TEXT_NULLCHAR)
            result += QChar(0xFFFD);
        else if (a.substr_types[i] == MD_TEXT_ENTITY)
            result += QTextDocumentFragment::fromHtml(part).toPlainText();
        else
            result += part;
    }
    return result;
}

class Renderer {
public:
    explicit Renderer(const MarkdownStyle &style) : s(style) {}

    QString render(const QString &markdown) {
        const QByteArray utf8 = markdown.toUtf8();
        MD_PARSER parser{};
        parser.flags = MD_DIALECT_GITHUB | MD_FLAG_LATEXMATHSPANS | MD_FLAG_WIKILINKS | MD_FLAG_HIGHLIGHT;
        parser.enter_block = [](MD_BLOCKTYPE t, void *d, void *u) { return static_cast<Renderer *>(u)->enterBlock(t, d); };
        parser.leave_block = [](MD_BLOCKTYPE t, void *d, void *u) { return static_cast<Renderer *>(u)->leaveBlock(t, d); };
        parser.enter_span = [](MD_SPANTYPE t, void *d, void *u) { return static_cast<Renderer *>(u)->enterSpan(t, d); };
        parser.leave_span = [](MD_SPANTYPE t, void *d, void *u) { return static_cast<Renderer *>(u)->leaveSpan(t, d); };
        parser.text = [](MD_TEXTTYPE t, const MD_CHAR *text, MD_SIZE size, void *u) {
            return static_cast<Renderer *>(u)->text(t, QString::fromUtf8(text, size));
        };
        md_parse(utf8.constData(), MD_SIZE(utf8.size()), &parser, this);
        return m_stack.join(QString());
    }

    // A fenced or indented code block: a full-width box, so the background
    // covers the whole block rather than only the characters.
    QString codeBox(const QString &code, const QString &language) const {
        QString body = code;
        while (body.endsWith('\n'))
            body.chop(1);
        return QStringLiteral("<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"%1\" bgcolor=\"%2\"><tr>"
                              "<td style=\"white-space:pre-wrap; font-family:'%3'; font-size:%4px; color:%5;\">"
                              "%6</td></tr></table>")
                   .arg(px(0.8)).arg(s.surface, s.monoFamily).arg(px(0.875)).arg(s.heading, highlightCode(body, language, s))
               + gap(0.8);
    }

    int px(double scale) const { return qRound(s.fontSize * scale); }

    // Vertical space as an empty line of a given size. Qt's rich text ignores
    // margins on tables, which is what every box on the page is made of.
    QString gap(double scale) const {
        return QStringLiteral("<p style=\"margin:0px; font-size:%1px; line-height:100%;\">&nbsp;</p>")
            .arg(px(scale));
    }

private:
    const MarkdownStyle &s;
    // Output buffers. Quotes, footnotes, and image alt text are collected in
    // their own buffer first, because how they open depends on what is inside.
    QStringList m_stack{QString()};
    bool m_inCodeBlock = false;
    QString m_code, m_codeLanguage;
    int m_imageDepth = 0;
    int m_listDepth = 0;

    QString &out() { return m_stack.last(); }

    // A box that directly follows a heading needs the heading's gap spelled
    // out, and a heading that follows a box must not double the box's gap.
    void box(const QString &html) {
        // Looks only at the tail: searching the whole page for every box
        // would make a long document take seconds to render.
        const QString &sofar = out();
        const int n = sofar.size();
        if (n >= 5 && sofar[n - 1] == '>' && sofar[n - 2].isDigit() && sofar[n - 3] == 'h' && sofar[n - 4] == '/')
            out() += gap(0.4);
        out() += html;
    }
    QString paragraphStyle() const {
        return QStringLiteral("margin-top:0px; margin-bottom:%1px; line-height:150%;").arg(px(1));
    }

    // A bar down the left edge with the content beside it, for quotes and
    // alerts. Qt's rich text has no border-left and only fills a table's own
    // background reliably, so the bar is an outer table in the bar's color
    // showing 4px past an inner table in the page's color.
    QString barBox(const QString &barColor, QString inner) const {
        // The last paragraph's gap would only make the bar overshoot the text.
        if (inner.endsWith(gap(0.8)))
            inner.chop(gap(0.8).size());
        const int last = inner.lastIndexOf(paragraphStyle());
        if (inner.endsWith("</p>") && last >= 0 && inner.indexOf("</p>", last) == inner.size() - 4)
            inner.replace(last, paragraphStyle().size(), QStringLiteral("margin-top:0px; margin-bottom:0px; line-height:150%;"));
        const QString table = QStringLiteral("<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"%1\"><tr>");
        return table.arg(barColor) + QStringLiteral("<td style=\"padding-left:4px;\">") + table.arg(s.background)
               + QStringLiteral("<td style=\"padding-left:%1px; padding-top:%2px; padding-bottom:%2px;\">%3</td></tr></table></td></tr></table>")
                     .arg(px(1)).arg(px(0.15)).arg(inner)
               + gap(0.8);
    }

    int enterBlock(MD_BLOCKTYPE type, void *detail) {
        switch (type) {
        case MD_BLOCK_QUOTE:
        case MD_BLOCK_ADMONITION:
        case MD_BLOCK_FOOTNOTE_DEF:
            m_stack.append(QString());
            break;
        case MD_BLOCK_UL:
        case MD_BLOCK_OL: {
            // Only the outermost list is followed by a paragraph gap.
            const QString style = QStringLiteral("margin-top:0px; margin-bottom:%1px;").arg(m_listDepth == 0 ? px(1) : 0);
            if (type == MD_BLOCK_UL)
                out() += QStringLiteral("<ul style=\"%1\">").arg(style);
            else
                out() += QStringLiteral("<ol start=\"%1\" style=\"%2\">")
                             .arg(static_cast<MD_BLOCK_OL_DETAIL *>(detail)->start).arg(style);
            ++m_listDepth;
            break;
        }
        case MD_BLOCK_LI: {
            const auto *li = static_cast<MD_BLOCK_LI_DETAIL *>(detail);
            // Qt draws a real checkbox marker for these two classes.
            const QString task = !li->is_task ? QString()
                               : li->task_mark == ' ' ? QStringLiteral(" class=\"unchecked\"") : QStringLiteral(" class=\"checked\"");
            out() += QStringLiteral("<li%1 style=\"margin-top:%2px; margin-bottom:%2px; line-height:150%;\">").arg(task).arg(px(0.25));
            break;
        }
        case MD_BLOCK_HR:
            out() += QStringLiteral("<hr/>") + gap(0.8);
            break;
        case MD_BLOCK_H: {
            static const double sizes[] = {2.0, 1.5, 1.25, 1.0, 0.875, 0.85};
            if (out().endsWith(gap(0.8)))
                out().chop(gap(0.8).size());
            const unsigned level = qBound(1u, static_cast<MD_BLOCK_H_DETAIL *>(detail)->level, 6u);
            out() += QStringLiteral("<h%1 style=\"font-size:%2px; font-weight:600; color:%3; margin-top:%4px; margin-bottom:%5px;\">")
                         .arg(level).arg(px(sizes[level - 1])).arg(level == 6 ? s.muted : s.heading)
                         .arg(px(level <= 2 ? 1.6 : 1.3)).arg(px(0.7));
            break;
        }
        case MD_BLOCK_CODE:
            m_inCodeBlock = true;
            m_code.clear();
            m_codeLanguage = attribute(static_cast<MD_BLOCK_CODE_DETAIL *>(detail)->lang);
            break;
        case MD_BLOCK_P:
            out() += QStringLiteral("<p style=\"%1\">").arg(paragraphStyle());
            break;
        case MD_BLOCK_TABLE:
            box(QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"%1\" "
                               "style=\"border-collapse:collapse; border-color:%2;\">")
                    .arg(px(0.5)).arg(s.border));
            break;
        case MD_BLOCK_TR:
            out() += QStringLiteral("<tr>");
            break;
        case MD_BLOCK_TH:
        case MD_BLOCK_TD: {
            const MD_ALIGN align = static_cast<MD_BLOCK_TD_DETAIL *>(detail)->align;
            const char *name = align == MD_ALIGN_CENTER ? "center" : align == MD_ALIGN_RIGHT ? "right" : "left";
            if (type == MD_BLOCK_TH)
                out() += QStringLiteral("<th align=\"%1\" style=\"color:%2; font-weight:600;\">")
                             .arg(QLatin1String(name), s.heading);
            else
                out() += QStringLiteral("<td align=\"%1\">").arg(QLatin1String(name));
            break;
        }
        case MD_BLOCK_FOOTNOTE_DEF_SECTION:
            enterBlock(MD_BLOCK_HR, nullptr);
            break;
        default:
            break;
        }
        return 0;
    }

    int leaveBlock(MD_BLOCKTYPE type, void *detail) {
        switch (type) {
        case MD_BLOCK_QUOTE: {
            const QString inner = m_stack.takeLast();
            box(barBox(s.border, inner));
            break;
        }
        case MD_BLOCK_ADMONITION: {
            const QString inner = m_stack.takeLast();
            const QString kind = attribute(static_cast<MD_BLOCK_ADMONITION_DETAIL *>(detail)->type).toLower();
            QString color = s.blue, title = QCoreApplication::translate("Markdown", "Note");
            if (kind == "tip") { color = s.green; title = QCoreApplication::translate("Markdown", "Tip"); }
            else if (kind == "important") { color = s.magenta; title = QCoreApplication::translate("Markdown", "Important"); }
            else if (kind == "warning") { color = s.yellow; title = QCoreApplication::translate("Markdown", "Warning"); }
            else if (kind == "caution") { color = s.red; title = QCoreApplication::translate("Markdown", "Caution"); }
            const QString heading = QStringLiteral("<p style=\"margin-top:0px; margin-bottom:%1px; color:%2; font-weight:600;\">%3</p>")
                                        .arg(px(0.5)).arg(color, escape(title));
            box(barBox(color, heading + inner));
            break;
        }
        case MD_BLOCK_FOOTNOTE_DEF: {
            QString inner = m_stack.takeLast();
            const auto *def = static_cast<MD_BLOCK_FOOTNOTE_DEF_DETAIL *>(detail);
            // The number leads the definition and links back to where the
            // footnote was first referenced.
            const QString number = QStringLiteral("<a name=\"fn-%1\" href=\"#fnref-%1-1\" style=\"color:%2; text-decoration:none;\">%1.</a> ")
                                       .arg(def->id).arg(s.accent);
            const int paragraph = inner.startsWith("<p ") ? inner.indexOf('>') + 1 : -1;
            if (paragraph > 0)
                inner.insert(paragraph, number);
            else
                inner = QStringLiteral("<p style=\"%1\">%2%3</p>").arg(paragraphStyle(), number, inner);
            out() += inner;
            break;
        }
        case MD_BLOCK_UL:
            --m_listDepth;
            out() += QStringLiteral("</ul>");
            break;
        case MD_BLOCK_OL:
            --m_listDepth;
            out() += QStringLiteral("</ol>");
            break;
        case MD_BLOCK_LI:
            out() += QStringLiteral("</li>");
            break;
        case MD_BLOCK_H:
            out() += QStringLiteral("</h%1>").arg(qBound(1u, static_cast<MD_BLOCK_H_DETAIL *>(detail)->level, 6u));
            break;
        case MD_BLOCK_CODE:
            m_inCodeBlock = false;
            box(codeBox(m_code, m_codeLanguage));
            break;
        case MD_BLOCK_P:
            out() += QStringLiteral("</p>");
            break;
        case MD_BLOCK_TABLE:
            out() += QStringLiteral("</table>") + gap(0.8);
            break;
        case MD_BLOCK_TR:
            out() += QStringLiteral("</tr>");
            break;
        case MD_BLOCK_TH:
            out() += QStringLiteral("</th>");
            break;
        case MD_BLOCK_TD:
            out() += QStringLiteral("</td>");
            break;
        default:
            break;
        }
        return 0;
    }

    QString inlineCodeStyle(const QString &color) const {
        return QStringLiteral("font-family:'%1'; font-size:%2px; background-color:%3; color:%4;")
            .arg(s.monoFamily).arg(px(0.875)).arg(s.surface, color);
    }

    int enterSpan(MD_SPANTYPE type, void *detail) {
        switch (type) {
        case MD_SPAN_EM: out() += QStringLiteral("<em>"); break;
        case MD_SPAN_STRONG: out() += QStringLiteral("<strong style=\"color:%1;\">").arg(s.heading); break;
        case MD_SPAN_DEL: out() += QStringLiteral("<s>"); break;
        case MD_SPAN_U:
        case MD_SPAN_INS: out() += QStringLiteral("<u>"); break;
        case MD_SPAN_SUPERSCRIPT: out() += QStringLiteral("<sup>"); break;
        case MD_SPAN_SUBSCRIPT: out() += QStringLiteral("<sub>"); break;
        case MD_SPAN_MARK:
            out() += QStringLiteral("<span style=\"background-color:%1; color:#000000;\">").arg(s.yellow);
            break;
        case MD_SPAN_CODE:
            out() += QStringLiteral("<code style=\"%1\">").arg(inlineCodeStyle(s.heading));
            break;
        case MD_SPAN_LATEXMATH:
        case MD_SPAN_LATEXMATH_DISPLAY:
            out() += QStringLiteral("<code style=\"%1\">").arg(inlineCodeStyle(s.cyan));
            break;
        case MD_SPAN_A: {
            const auto *a = static_cast<MD_SPAN_A_DETAIL *>(detail);
            const QString title = attribute(a->title);
            out() += QStringLiteral("<a href=\"%1\"%2 style=\"color:%3; text-decoration:none;\">")
                         .arg(escape(attribute(a->href)),
                              title.isEmpty() ? QString() : QStringLiteral(" title=\"%1\"").arg(escape(title)), s.accent);
            break;
        }
        case MD_SPAN_WIKILINK:
            out() += QStringLiteral("<a href=\"%1\" style=\"color:%2; text-decoration:none;\">")
                         .arg(escape(attribute(static_cast<MD_SPAN_WIKILINK_DETAIL *>(detail)->target)), s.accent);
            break;
        case MD_SPAN_FOOTNOTE_REF: {
            const auto *ref = static_cast<MD_SPAN_FOOTNOTE_REF_DETAIL *>(detail);
            out() += QStringLiteral("<sup><a name=\"fnref-%1-%2\" href=\"#fn-%1\" style=\"color:%3; text-decoration:none;\">%1</a></sup>")
                         .arg(ref->id).arg(ref->ref_id).arg(s.accent);
            break;
        }
        case MD_SPAN_IMG:
            // The alt text arrives as ordinary text and spans; collect it.
            ++m_imageDepth;
            m_stack.append(QString());
            break;
        default:
            break;
        }
        return 0;
    }

    int leaveSpan(MD_SPANTYPE type, void *detail) {
        switch (type) {
        case MD_SPAN_EM: out() += QStringLiteral("</em>"); break;
        case MD_SPAN_STRONG: out() += QStringLiteral("</strong>"); break;
        case MD_SPAN_DEL: out() += QStringLiteral("</s>"); break;
        case MD_SPAN_U:
        case MD_SPAN_INS: out() += QStringLiteral("</u>"); break;
        case MD_SPAN_SUPERSCRIPT: out() += QStringLiteral("</sup>"); break;
        case MD_SPAN_SUBSCRIPT: out() += QStringLiteral("</sub>"); break;
        case MD_SPAN_MARK: out() += QStringLiteral("</span>"); break;
        case MD_SPAN_CODE:
        case MD_SPAN_LATEXMATH:
        case MD_SPAN_LATEXMATH_DISPLAY: out() += QStringLiteral("</code>"); break;
        case MD_SPAN_A:
        case MD_SPAN_WIKILINK: out() += QStringLiteral("</a>"); break;
        case MD_SPAN_IMG: {
            --m_imageDepth;
            static const QRegularExpression tags("<[^>]*>");
            const QString alt = m_stack.takeLast().remove(tags);
            const auto *img = static_cast<MD_SPAN_IMG_DETAIL *>(detail);
            const QString source = attribute(img->src), title = attribute(img->title);
            out() += QStringLiteral("<img src=\"%1\" alt=\"%2\"%3%4/>")
                         .arg(escape(source), alt,
                              title.isEmpty() ? QString() : QStringLiteral(" title=\"%1\"").arg(escape(title)),
                              imageSize(source));
            break;
        }
        default:
            break;
        }
        return 0;
    }

    // Qt draws images at their natural size, so one wider than the page
    // would run off its edge. Local files can be measured and scaled to fit.
    QString imageSize(const QString &source) const {
        const QUrl url(source);
        if (!url.scheme().isEmpty() && !url.isLocalFile())
            return {};
        const QString path = url.isLocalFile() ? url.toLocalFile() : QUrl::fromPercentEncoding(source.toUtf8());
        const QString resolved = QDir::isAbsolutePath(path) || s.baseDirectory.isEmpty()
                               ? path : QDir(s.baseDirectory).filePath(path);
        const QSize size = QImageReader(resolved).size();
        if (!size.isValid() || size.width() <= s.maxImageWidth || s.maxImageWidth <= 0)
            return {};
        return QStringLiteral(" width=\"%1\" height=\"%2\"")
            .arg(s.maxImageWidth).arg(qRound(double(size.height()) * s.maxImageWidth / size.width()));
    }

    int text(MD_TEXTTYPE type, const QString &text) {
        if (m_inCodeBlock) {
            m_code += text;
            return 0;
        }
        switch (type) {
        case MD_TEXT_NULLCHAR: out() += QChar(0xFFFD); break;
        case MD_TEXT_BR: out() += m_imageDepth ? QStringLiteral(" ") : QStringLiteral("<br/>"); break;
        case MD_TEXT_SOFTBR: out() += m_imageDepth ? QStringLiteral(" ") : QStringLiteral("\n"); break;
        case MD_TEXT_ENTITY: out() += text; break;
        case MD_TEXT_HTML:
            if (!m_imageDepth)
                out() += text;
            break;
        default: out() += escape(text); break;
        }
        return 0;
    }
};

} // namespace

QString renderMarkdown(const QString &markdown, const MarkdownStyle &style) {
    QString source = markdown;
    source.replace("\r\n", "\n");
    Renderer renderer(style);

    // YAML front matter is metadata, not content; show it as a block of YAML
    // rather than letting it parse as a rule followed by a heading.
    QString frontMatter;
    if (source.startsWith("---\n")) {
        static const QRegularExpression closing("\n(---|\\.\\.\\.)[ \t]*(\n|$)");
        const auto match = closing.match(source, 3);
        if (match.hasMatch()) {
            frontMatter = renderer.codeBox(source.mid(4, match.capturedStart() - 4 + 1), "yaml");
            source = source.mid(match.capturedEnd());
        }
    }
    return frontMatter + renderer.render(source);
}
