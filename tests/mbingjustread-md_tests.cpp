#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QtTest>

#include "backend.h"
#include "filepicker.h"
#include "highlighter.h"
#include "markdown.h"
#include "theme.h"
#include "translator.h"

// Hands back a fixed path, as if the user had picked it in the dialog.
class FakePicker : public FilePicker {
    Q_OBJECT

public:
    QString answer;
    void pick(const QString &, const QString &) override { emit picked(answer); }
};

class ReaderTests : public QObject {
    Q_OBJECT

private slots:
    // Rendering ------------------------------------------------------------

    void rendersHeadingsAndInlineStyles()
    {
        const QString html = renderMarkdown("# Title\n\nSome **bold**, *soft*, ~~gone~~ and `code`.", {});
        QVERIFY(html.contains("<h1"));
        QVERIFY(html.contains("Title</h1>"));
        QVERIFY(html.contains("bold</strong>"));
        QVERIFY(html.contains("<em>soft</em>"));
        QVERIFY(html.contains("<s>gone</s>"));
        QVERIFY(html.contains("code</code>"));
    }

    void escapesTextButKeepsRawHtml()
    {
        const QString html = renderMarkdown("1 < 2 & 3 <kbd>Ctrl</kbd>", {});
        QVERIFY(html.contains("1 &lt; 2 &amp; 3"));
        QVERIFY(html.contains("<kbd>Ctrl</kbd>"));
    }

    void rendersTablesWithAlignment()
    {
        const QString html = renderMarkdown("| a | b |\n| :-: | --: |\n| 1 | 2 |\n", {});
        QVERIFY(html.contains("<th align=\"center\""));
        QVERIFY(html.contains("<td align=\"right\">2</td>"));
    }

    void rendersTaskLists()
    {
        const QString html = renderMarkdown("- [x] done\n- [ ] todo\n", {});
        QVERIFY(html.contains("<li class=\"checked\""));
        QVERIFY(html.contains("<li class=\"unchecked\""));
    }

    void keepsOrderedListStart()
    {
        QVERIFY(renderMarkdown("7. seven\n8. eight\n", {}).contains("<ol start=\"7\""));
    }

    void rendersAlertsAndFootnotes()
    {
        const QString html = renderMarkdown("> [!WARNING]\n> Careful.\n\nText[^a].\n\n[^a]: The note.\n", {});
        QVERIFY(html.contains("Warning</p>"));
        QVERIFY(html.contains("Careful."));
        QVERIFY(html.contains("href=\"#fn-1\""));
        QVERIFY(html.contains("name=\"fn-1\""));
        QVERIFY(html.contains("The note."));
    }

    void showsFrontMatterAsYaml()
    {
        const QString html = renderMarkdown("---\ntitle: Hello\n---\n\n# Body\n", {});
        QVERIFY(html.contains("title</span>"));
        QVERIFY(html.contains("Body</h1>"));
        QVERIFY(!html.contains("<h2"));
    }

    void highlightsCode()
    {
        MarkdownStyle s;
        const QString cpp = highlightCode("return 42; // done\nconst char *s = \"hi\";", "cpp", s);
        QVERIFY(cpp.contains(QString("<span style=\"color:%1;\">return</span>").arg(s.magenta)));
        QVERIFY(cpp.contains(QString("<span style=\"color:%1;\">42</span>").arg(s.orange)));
        QVERIFY(cpp.contains(QString("<span style=\"color:%1;\">// done</span>").arg(s.muted)));
        QVERIFY(cpp.contains(QString("<span style=\"color:%1;\">&quot;hi&quot;</span>").arg(s.green)));

        // Unknown languages and markup still come back safely escaped.
        QCOMPARE(highlightCode("a < b", "nonsense", s), QString("a &lt; b"));
        QVERIFY(!highlightCode("<div class=\"x\">", "html", s).contains("<div"));
        // A Rust lifetime is not an unterminated string.
        QVERIFY(!highlightCode("fn f<'a>(x: &'a str)", "rust", s).contains(s.green));
        // A # inside a word is not a comment.
        QVERIFY(!highlightCode("echo $#", "bash", s).contains(s.muted));
    }

    void scalesWideLocalImages()
    {
        QTemporaryDir dir;
        QImage image(1600, 800, QImage::Format_RGB32);
        image.fill(Qt::red);
        QVERIFY(image.save(dir.filePath("wide.png")));

        MarkdownStyle s;
        s.baseDirectory = dir.path();
        s.maxImageWidth = 400;
        const QString html = renderMarkdown("![A *wide* one](wide.png)", s);
        QVERIFY(html.contains("width=\"400\" height=\"200\""));
        QVERIFY(html.contains("alt=\"A wide one\""));
    }

    void rendersTheShowcaseIntoARealDocument()
    {
        QFile file(QString(FIXTURES) + "/showcase.md");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QTextDocument document;
        document.setHtml(renderMarkdown(QString::fromUtf8(file.readAll()), {}));
        const QString text = document.toPlainText();
        QVERIFY(text.contains("Markdown showcase"));
        QVERIFY(text.contains("Parsing follows the CommonMark specification."));
        QVERIFY(!text.contains("[^spec]"));
        QVERIFY(!text.contains("[!NOTE]"));
    }

    void rendersALongDocumentQuickly()
    {
        QFile file(QString(FIXTURES) + "/showcase.md");
        QVERIFY(file.open(QIODevice::ReadOnly));
        // About 600 KB and 30,000 lines. Rendering must stay linear in the
        // size of the document, so this takes a fraction of a second.
        const QString markdown = QString::fromUtf8(file.readAll()).repeated(400);
        QElapsedTimer timer;
        timer.start();
        const QString html = renderMarkdown(markdown, {});
        QVERIFY(!html.isEmpty());
        QVERIFY2(timer.elapsed() < 3000, qPrintable(QString("took %1 ms").arg(timer.elapsed())));
    }

    // Backend --------------------------------------------------------------

    void opensAFileAndBuildsTheOutline()
    {
        QTemporaryDir dir;
        write(dir.filePath("a.md"), "# One\n\ntext\n\n## Two\n\n## Two\n\nmore needle text\n");
        Theme theme(dir.path());
        Backend backend(&theme, nullptr, nullptr, false);
        QSignalSpy loaded(&backend, &Backend::loaded);
        backend.open(dir.filePath("a.md"));
        QCOMPARE(loaded.count(), 1);
        QCOMPARE(loaded.first().first().toBool(), false);
        QCOMPARE(backend.fileName(), QString("a.md"));
        QVERIFY(backend.error().isEmpty());

        QTextDocument document;
        document.setHtml(backend.html());
        backend.indexDocument(&document);
        const auto outline = backend.outline();
        QCOMPARE(outline.size(), 3);
        QCOMPARE(outline[0].toMap()["title"].toString(), QString("One"));
        QCOMPARE(outline[1].toMap()["level"].toInt(), 2);
        QCOMPARE(backend.headingAt(0), 0);
        QCOMPARE(backend.headingAt(outline[2].toMap()["position"].toInt() + 1), 2);

        // Heading links, with GitHub's suffix for a repeated heading.
        QSignalSpy scroll(&backend, &Backend::scrollToPosition);
        backend.activateLink("#two-1");
        QCOMPARE(scroll.count(), 1);
        QCOMPARE(scroll.first().first().toInt(), outline[2].toMap()["position"].toInt());
        QSignalSpy notice(&backend, &Backend::notice);
        backend.activateLink("#missing");
        QCOMPARE(notice.count(), 1);

        QSignalSpy match(&backend, &Backend::matchSelected);
        backend.search("NEEDLE");
        QCOMPARE(backend.matchCount(), 1);
        QCOMPARE(match.count(), 1);
        backend.search("absent");
        QCOMPARE(backend.matchCount(), 0);
    }

    void reportsAMissingFile()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        Backend backend(&theme, nullptr, nullptr, false);
        backend.open(dir.filePath("nope.md"));
        QVERIFY(!backend.hasDocument());
        QVERIFY(backend.error().contains("nope.md"));
    }

    void followsLinksToOtherFilesAndBack()
    {
        QTemporaryDir dir;
        write(dir.filePath("a.md"), "[next](b.md) [[c]]\n");
        write(dir.filePath("b.md"), "# B\n");
        write(dir.filePath("c.md"), "# C\n");
        Theme theme(dir.path());
        Backend backend(&theme, nullptr, nullptr, false);
        backend.open(dir.filePath("a.md"));
        QVERIFY(!backend.canGoBack());

        backend.activateLink("b.md");
        QCOMPARE(backend.fileName(), QString("b.md"));
        QVERIFY(backend.canGoBack());
        backend.back();
        QCOMPARE(backend.fileName(), QString("a.md"));
        QVERIFY(backend.canGoForward());
        backend.forward();
        QCOMPARE(backend.fileName(), QString("b.md"));

        // A wiki link names the page without its extension.
        backend.activateLink("c");
        QCOMPARE(backend.fileName(), QString("c.md"));
    }

    void reloadsWhenTheFileChanges()
    {
        QTemporaryDir dir;
        write(dir.filePath("a.md"), "first\n");
        Theme theme(dir.path());
        Backend backend(&theme, nullptr, nullptr, false);
        backend.open(dir.filePath("a.md"));
        QSignalSpy loaded(&backend, &Backend::loaded);

        // Saved the way editors do it: a new file renamed over the old one.
        write(dir.filePath("a.md.tmp"), "second\n");
        QFile::remove(dir.filePath("a.md"));
        QVERIFY(QFile::rename(dir.filePath("a.md.tmp"), dir.filePath("a.md")));
        QTRY_VERIFY(backend.html().contains("second"));
        QCOMPARE(loaded.last().first().toBool(), true);
    }

    void zoomsWithinLimits()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        Backend backend(&theme, nullptr, nullptr, false);
        QCOMPARE(backend.fontSize(), 16);
        backend.zoomIn();
        QCOMPARE(backend.fontSize(), 17);
        for (int i = 0; i < 100; ++i)
            backend.zoomOut();
        QCOMPARE(backend.fontSize(), 10);
        backend.zoomReset();
        QCOMPARE(backend.fontSize(), 16);
    }

    void opensWhatThePickerReturns()
    {
        QTemporaryDir dir;
        write(dir.filePath("picked.md"), "# Picked\n");
        Theme theme(dir.path());
        FakePicker picker;
        picker.answer = dir.filePath("picked.md");
        Backend backend(&theme, &picker, nullptr, false);
        backend.openDialog();
        QCOMPARE(backend.fileName(), QString("picked.md"));
    }

    void slugsLikeGitHub()
    {
        QCOMPARE(Backend::slug("Hello, World!"), QString("hello-world"));
        QCOMPARE(Backend::slug("  What's new in v2.0?  "), QString("whats-new-in-v20"));
        QCOMPARE(Backend::slug("snake_case and kebab-case"), QString("snake_case-and-kebab-case"));
    }

    // Theme ----------------------------------------------------------------

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QVERIFY(theme.dark());
        QCOMPARE(theme.background(), QString("#0e0e10"));
    }

    void themeReadsThePalette()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml",
              "mode = \"light\"\naccent = \"#112233\"\nbackground = \"#ffffff\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QVERIFY(!theme.dark());
        QCOMPARE(theme.background(), QString("#ffffff"));
        // Colors the theme leaves out come from the light defaults.
        QCOMPARE(theme.foreground(), QString("#2e2e33"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n");
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        write(response, "{\"int\":0}");

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        write(response, "{\"int\":6}");
        QTRY_COMPARE(theme.radius(), 6);
    }

    // Translator -----------------------------------------------------------

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Reload").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void shipsAnIndonesianCatalog()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "id\n");
        Translator translator(dir.path() + "/language", QString(FIXTURES) + "/../../src/i18n");
        QCOMPARE(translator.translate("Main", "Reload"), QString("Muat ulang"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(ReaderTests)
#include "mbingjustread-md_tests.moc"
