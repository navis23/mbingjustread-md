#include "highlighter.h"

#include <QHash>
#include <QSet>
#include <QStringList>

#include "markdown.h"

namespace {

enum class Kind { Code, Markup, Diff };

struct Syntax {
    Kind kind = Kind::Code;
    QStringList lineComments;
    QString blockStart, blockEnd;
    QSet<QString> keywords;
    bool caseInsensitive = false;
    bool singleQuoteStrings = true; // false: 'x' is only ever one character
    bool backtickStrings = false;
    bool tripleQuoteStrings = false;
    bool keys = false;              // `name:` or `name =` at the start of a line
    bool types = false;             // Capitalized identifiers are type names
};

QSet<QString> words(const char *list) {
    const auto parts = QString::fromLatin1(list).split(' ', Qt::SkipEmptyParts);
    return QSet<QString>(parts.begin(), parts.end());
}

const QHash<QString, Syntax> &syntaxes() {
    static const QHash<QString, Syntax> table = [] {
        QHash<QString, Syntax> t;
        auto add = [&t](const char *names, const Syntax &syntax) {
            for (const auto &name : QString::fromLatin1(names).split(' '))
                t.insert(name, syntax);
        };

        const auto cWords = words(
            "abstract as async await auto bool boolean break case catch chan char class const constexpr continue "
            "default defer delete do double dyn else enum explicit export extends extern false final finally float fn "
            "for foreach friend from fun func function go goto guard if impl implements import in inline instanceof "
            "int interface internal is let long loop match mod module mut namespace new nil noexcept null nullptr "
            "object of operator override package private protected protocol pub public range readonly ref return "
            "select self short signed sizeof static struct super switch template this throw throws trait true try "
            "type typedef typename typeof undefined union unsafe unsigned use using val var virtual void volatile "
            "when where while yield");

        Syntax c;
        c.lineComments = {"//"};
        c.blockStart = "/*";
        c.blockEnd = "*/";
        c.keywords = cWords;
        c.singleQuoteStrings = false;
        c.types = true;
        add("c h cpp c++ cc cxx hpp java go golang rust rs cs csharp kotlin kt swift scala zig objc d", c);

        Syntax js = c;
        js.singleQuoteStrings = true;
        js.backtickStrings = true;
        add("js javascript jsx ts typescript tsx mjs cjs dart php qml groovy", js);

        Syntax css;
        css.blockStart = "/*";
        css.blockEnd = "*/";
        css.keys = true;
        add("css scss less", css);

        Syntax hash;
        hash.lineComments = {"#"};
        hash.keywords = words(
            "and as assert async await begin break case class continue def defined del do done elif else elsif end "
            "ensure esac except export False fi finally for from function global if import in is lambda local module "
            "next nil None nonlocal not or pass raise require rescue return self then True try unless until when "
            "while with yield true false null");
        Syntax python = hash;
        python.tripleQuoteStrings = true;
        python.types = true;
        add("python py python3 ruby rb perl pl r elixir ex nim julia", python);
        Syntax shell = hash;
        shell.backtickStrings = true;
        add("sh bash shell zsh fish console terminal makefile make dockerfile docker nix powershell ps1", shell);
        Syntax config = hash;
        config.keys = true;
        config.keywords = words("true false null yes no on off");
        add("yaml yml toml ini conf cfg properties env", config);

        Syntax json;
        json.keywords = words("true false null");
        json.keys = true;
        json.lineComments = {"//"};
        add("json jsonc json5", json);

        Syntax sql;
        sql.lineComments = {"--"};
        sql.blockStart = "/*";
        sql.blockEnd = "*/";
        sql.caseInsensitive = true;
        sql.keywords = words(
            "add all alter and as asc begin between by case check column commit constraint create cross database "
            "default delete desc distinct drop else end exists foreign from full group having if in index inner "
            "insert into is join key left like limit not null offset on or order outer primary references returning "
            "right rollback select set table then union unique update values view when where with");
        add("sql postgres postgresql mysql sqlite plsql", sql);

        Syntax lua;
        lua.lineComments = {"--"};
        lua.blockStart = "--[[";
        lua.blockEnd = "]]";
        lua.keywords = words("and break do else elseif end false for function goto if in local nil not or repeat "
                             "return then true until while");
        add("lua", lua);

        Syntax haskell;
        haskell.lineComments = {"--"};
        haskell.blockStart = "{-";
        haskell.blockEnd = "-}";
        haskell.singleQuoteStrings = false;
        haskell.types = true;
        haskell.keywords = words("case class data deriving do else if import in infix instance let module newtype of "
                                 "then type where");
        add("haskell hs elm", haskell);

        Syntax markup;
        markup.kind = Kind::Markup;
        add("html htm xml svg xhtml vue svelte", markup);

        Syntax diff;
        diff.kind = Kind::Diff;
        add("diff patch", diff);
        return t;
    }();
    return table;
}

class Writer {
public:
    void plain(QStringView text) { m_plain += text; }
    void colored(const QString &color, QStringView text) {
        flush();
        m_out += QStringLiteral("<span style=\"color:%1;\">%2</span>").arg(color, text.toString().toHtmlEscaped());
    }
    QString result() {
        flush();
        return m_out;
    }

private:
    void flush() {
        if (!m_plain.isEmpty())
            m_out += m_plain.toHtmlEscaped();
        m_plain.clear();
    }
    QString m_out, m_plain;
};

bool isWord(QChar c) { return c.isLetterOrNumber() || c == '_'; }

// Only whitespace and list dashes sit between the line start and `at`.
bool startsLine(const QString &code, int at) {
    for (int i = at - 1; i >= 0 && code[i] != '\n'; --i)
        if (!code[i].isSpace() && code[i] != '-')
            return false;
    return true;
}

// The end of a quoted string that opens at `start`, honoring backslashes.
// Ordinary quotes stop at the end of the line when they are never closed.
int stringEnd(const QString &code, int start, const QString &quote, bool multiline) {
    int i = start + quote.size();
    while (i < code.size()) {
        if (code[i] == '\\') {
            i += 2;
            continue;
        }
        if (QStringView(code).mid(i).startsWith(quote))
            return i + quote.size();
        if (code[i] == '\n' && !multiline)
            return i;
        ++i;
    }
    return code.size();
}

QString highlightGeneric(const QString &code, const Syntax &syntax, const MarkdownStyle &s) {
    Writer w;
    const QStringView view(code);
    int i = 0;
    while (i < code.size()) {
        const QChar c = code[i];
        const QStringView rest = view.mid(i);

        if (!syntax.blockStart.isEmpty() && rest.startsWith(syntax.blockStart)) {
            int end = code.indexOf(syntax.blockEnd, i + syntax.blockStart.size());
            end = end < 0 ? code.size() : end + syntax.blockEnd.size();
            w.colored(s.muted, view.mid(i, end - i));
            i = end;
            continue;
        }

        bool comment = false;
        for (const auto &marker : syntax.lineComments) {
            // A `#` in the middle of a word ($#, a#b, a URL fragment) is not
            // a comment.
            if (!rest.startsWith(marker) || (marker == "#" && i > 0 && !code[i - 1].isSpace()))
                continue;
            int end = code.indexOf('\n', i);
            end = end < 0 ? code.size() : end;
            w.colored(s.muted, view.mid(i, end - i));
            i = end;
            comment = true;
            break;
        }
        if (comment)
            continue;

        if (c == '"' || c == '\'' || c == '`') {
            int end = -1;
            if (syntax.tripleQuoteStrings && c != '`' && rest.startsWith(QString(3, c)))
                end = stringEnd(code, i, QString(3, c), true);
            else if (c == '"')
                end = stringEnd(code, i, "\"", false);
            else if (c == '`' && syntax.backtickStrings)
                end = stringEnd(code, i, "`", true);
            else if (c == '\'' && syntax.singleQuoteStrings)
                end = stringEnd(code, i, "'", false);
            else if (c == '\'') {
                // A character literal: 'x' or an escape like '\n'. Anything
                // else is a lifetime or a prime and stays as it is.
                if (i + 2 < code.size() && code[i + 1] != '\\' && code[i + 2] == '\'')
                    end = i + 3;
                else if (i + 1 < code.size() && code[i + 1] == '\\') {
                    const int close = code.indexOf('\'', i + 3);
                    if (close > 0 && close - i <= 10)
                        end = close + 1;
                }
            }
            if (end > i) {
                // In JSON and YAML a string before a colon names a key.
                int after = end;
                while (after < code.size() && code[after] == ' ')
                    ++after;
                const bool key = syntax.keys && after < code.size() && code[after] == ':';
                w.colored(key ? s.blue : s.green, view.mid(i, end - i));
                i = end;
                continue;
            }
        }

        if (c.isDigit() && (i == 0 || !isWord(code[i - 1]))) {
            int end = i + 1;
            while (end < code.size() && (isWord(code[end]) || (code[end] == '.' && end + 1 < code.size() && code[end + 1].isDigit())))
                ++end;
            w.colored(s.orange, view.mid(i, end - i));
            i = end;
            continue;
        }

        if (c.isLetter() || c == '_') {
            int end = i + 1;
            while (end < code.size() && (isWord(code[end]) || (syntax.keys && (code[end] == '-' || code[end] == '.'))))
                ++end;
            const QString word = code.mid(i, end - i);
            int after = end;
            while (after < code.size() && code[after] == ' ')
                ++after;
            const QChar next = after < code.size() ? code[after] : QChar();

            if (syntax.keys && (next == ':' || next == '=') && startsLine(code, i))
                w.colored(s.blue, word);
            else if (syntax.keywords.contains(syntax.caseInsensitive ? word.toLower() : word))
                w.colored(s.magenta, word);
            else if (next == '(' && after == end)
                w.colored(s.blue, word);
            else if (syntax.types && word[0].isUpper())
                w.colored(s.yellow, word);
            else
                w.plain(word);
            i = end;
            continue;
        }

        w.plain(view.mid(i, 1));
        ++i;
    }
    return w.result();
}

QString highlightMarkup(const QString &code, const MarkdownStyle &s) {
    Writer w;
    const QStringView view(code);
    int i = 0;
    while (i < code.size()) {
        if (view.mid(i).startsWith(QLatin1String("<!--"))) {
            int end = code.indexOf("-->", i + 4);
            end = end < 0 ? code.size() : end + 3;
            w.colored(s.muted, view.mid(i, end - i));
            i = end;
            continue;
        }
        if (code[i] != '<') {
            w.plain(view.mid(i, 1));
            ++i;
            continue;
        }
        // Inside a tag: its name, then attribute names and quoted values.
        w.plain(view.mid(i, 1));
        ++i;
        bool nameDone = false;
        while (i < code.size() && code[i] != '>') {
            const QChar c = code[i];
            if (c == '"' || c == '\'') {
                const int end = stringEnd(code, i, QString(c), true);
                w.colored(s.green, view.mid(i, end - i));
                i = end;
            } else if (c.isLetter() || c == '_' || c == ':') {
                int end = i + 1;
                while (end < code.size() && (isWord(code[end]) || code[end] == '-' || code[end] == ':' || code[end] == '.'))
                    ++end;
                w.colored(nameDone ? s.yellow : s.magenta, view.mid(i, end - i));
                nameDone = true;
                i = end;
            } else {
                w.plain(view.mid(i, 1));
                ++i;
            }
        }
    }
    return w.result();
}

QString highlightDiff(const QString &code, const MarkdownStyle &s) {
    Writer w;
    const auto lines = code.split('\n');
    for (int n = 0; n < lines.size(); ++n) {
        const QString &line = lines[n];
        if (line.startsWith("@@"))
            w.colored(s.cyan, line);
        else if (line.startsWith('+'))
            w.colored(s.green, line);
        else if (line.startsWith('-'))
            w.colored(s.red, line);
        else
            w.plain(line);
        if (n + 1 < lines.size())
            w.plain(u"\n");
    }
    return w.result();
}

} // namespace

QString highlightCode(const QString &code, const QString &language, const MarkdownStyle &style) {
    const auto it = syntaxes().constFind(language.toLower());
    if (it == syntaxes().constEnd())
        return code.toHtmlEscaped();
    switch (it->kind) {
    case Kind::Markup: return highlightMarkup(code, style);
    case Kind::Diff: return highlightDiff(code, style);
    default: return highlightGeneric(code, *it, style);
    }
}
