#include "backend.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QQuickTextDocument>
#include <QSettings>
#include <QTextBlock>
#include <QTextCursor>

#include "filepicker.h"
#include "markdown.h"
#include "theme.h"

namespace {

const int MinFontSize = 10, MaxFontSize = 40, DefaultFontSize = 16;
// Far beyond any real document; it only stops a mistaken click on a huge
// log or binary from freezing the window.
const qint64 MaxFileSize = 64 * 1024 * 1024;
const int MaxMatches = 10000;

bool looksLikeMarkdown(const QString &path) {
    static const QStringList suffixes{"md", "markdown", "mdown", "mkd", "mkdn", "mdx", "txt", ""};
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

} // namespace

Backend::Backend(Theme *theme, FilePicker *picker, QObject *parent, bool persist)
    : QObject(parent), m_theme(theme), m_picker(picker), m_persist(persist) {
    if (m_persist)
        m_fontSize = qBound(MinFontSize, QSettings().value("fontSize", DefaultFontSize).toInt(), MaxFontSize);

    // Editors save by writing a new file over the old one, which drops the
    // watch and fires several events; wait for them to settle, then re-arm.
    m_reloadDebounce.setSingleShot(true);
    m_reloadDebounce.setInterval(120);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_reloadDebounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_reloadDebounce.start(); });
    connect(&m_reloadDebounce, &QTimer::timeout, this, &Backend::reload);

    // Images are scaled to the page width, so a resize re-renders them, but
    // only once the window has stopped moving.
    m_widthDebounce.setSingleShot(true);
    m_widthDebounce.setInterval(150);
    connect(&m_widthDebounce, &QTimer::timeout, this, [this] {
        if (hasDocument() && (m_source.contains("![") || m_source.contains("<img", Qt::CaseInsensitive)))
            render(true);
    });

    if (m_theme)
        connect(m_theme, &Theme::changed, this, [this] {
            if (hasDocument())
                render(true);
        });
    if (m_picker) {
        connect(m_picker, &FilePicker::picked, this, &Backend::open);
        connect(m_picker, &FilePicker::failed, this, &Backend::notice);
    }
}

QString Backend::fileName() const { return m_filePath.isEmpty() ? QString() : QFileInfo(m_filePath).fileName(); }

QUrl Backend::baseUrl() const {
    return m_filePath.isEmpty() ? QUrl() : QUrl::fromLocalFile(QFileInfo(m_filePath).absolutePath() + "/");
}

void Backend::setContentWidth(int width) {
    if (width == m_contentWidth || width <= 0)
        return;
    m_contentWidth = width;
    emit contentWidthChanged();
    m_widthDebounce.start();
}

void Backend::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void Backend::open(const QString &pathOrUrl) {
    if (pathOrUrl.isEmpty())
        return;
    const QString given = pathOrUrl.startsWith("file:") ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    const QString path = QFileInfo(given).absoluteFilePath();
    if (path == m_filePath) {
        reload();
        return;
    }
    if (!load(path, false))
        return;
    // Opening from the middle of the history drops what was ahead of it.
    m_history = m_history.mid(0, m_historyIndex + 1);
    m_history.append(path);
    m_historyIndex = m_history.size() - 1;
    emit historyChanged();
}

void Backend::openDialog() {
    if (!m_picker) {
        emit notice(tr("No file chooser is available."));
        return;
    }
    m_picker->pick(tr("Open Markdown file"), m_filePath.isEmpty() ? QDir::homePath() : QFileInfo(m_filePath).absolutePath());
}

void Backend::reload() {
    if (m_filePath.isEmpty())
        return;
    watch();
    load(m_filePath, true);
}

void Backend::back() {
    if (!canGoBack())
        return;
    if (load(m_history[m_historyIndex - 1], false)) {
        --m_historyIndex;
        emit historyChanged();
    }
}

void Backend::forward() {
    if (!canGoForward())
        return;
    if (load(m_history[m_historyIndex + 1], false)) {
        ++m_historyIndex;
        emit historyChanged();
    }
}

bool Backend::load(const QString &path, bool sameFile) {
    QFile file(path);
    const QFileInfo info(path);
    QString problem;
    if (info.isDir())
        problem = tr("%1 is a folder, not a file.").arg(path);
    else if (!info.exists())
        problem = tr("%1 does not exist.").arg(path);
    else if (info.size() > MaxFileSize)
        problem = tr("%1 is too large to read (over 64 MB).").arg(path);
    else if (!file.open(QIODevice::ReadOnly))
        problem = tr("%1 could not be read: %2").arg(path, file.errorString());

    if (!problem.isEmpty()) {
        // A file that vanishes while it is on screen keeps showing its last
        // contents; say so rather than blanking the page mid-read.
        if (sameFile || hasDocument())
            emit notice(problem);
        else
            setError(problem);
        return false;
    }

    const QString source = QString::fromUtf8(file.readAll());
    if (sameFile && source == m_source)
        return true;

    setError({});
    m_source = source;
    if (path != m_filePath) {
        m_filePath = path;
        watch();
        emit fileChanged();
    }
    render(sameFile);
    return true;
}

void Backend::watch() {
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);
    if (m_filePath.isEmpty())
        return;
    const QFileInfo info(m_filePath);
    if (info.exists())
        m_watcher.addPath(m_filePath);
    if (QFileInfo::exists(info.absolutePath()))
        m_watcher.addPath(info.absolutePath());
}

void Backend::render(bool sameFile) {
    MarkdownStyle style;
    if (m_theme) {
        style.background = m_theme->background();
        style.foreground = m_theme->foreground();
        style.heading = m_theme->brightForeground();
        style.muted = m_theme->mutedForeground();
        style.accent = m_theme->accent();
        style.surface = m_theme->surface();
        style.border = m_theme->border();
        style.red = m_theme->color("red");
        style.green = m_theme->color("green");
        style.yellow = m_theme->color("yellow");
        style.orange = m_theme->color("orange");
        style.blue = m_theme->color("blue");
        style.magenta = m_theme->color("magenta");
        style.cyan = m_theme->color("cyan");
    }
    style.monoFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    style.fontSize = m_fontSize;
    style.maxImageWidth = m_contentWidth;
    style.baseDirectory = QFileInfo(m_filePath).absolutePath();
    m_html = renderMarkdown(m_source, style);
    emit loaded(sameFile);
}

void Backend::setFontSize(int size) {
    size = qBound(MinFontSize, size, MaxFontSize);
    if (size == m_fontSize)
        return;
    m_fontSize = size;
    if (m_persist)
        QSettings().setValue("fontSize", m_fontSize);
    emit fontSizeChanged();
    if (hasDocument())
        render(true);
}

void Backend::zoomIn() { setFontSize(m_fontSize + 1); }
void Backend::zoomOut() { setFontSize(m_fontSize - 1); }
void Backend::zoomReset() { setFontSize(DefaultFontSize); }

QString Backend::slug(const QString &title) {
    QString result;
    for (const QChar c : title.trimmed().toLower()) {
        if (c.isLetterOrNumber() || c == '-' || c == '_')
            result += c;
        else if (c.isSpace())
            result += '-';
    }
    return result;
}

void Backend::attachDocument(QQuickTextDocument *document) {
    indexDocument(document ? document->textDocument() : nullptr);
}

void Backend::indexDocument(QTextDocument *document) {
    m_document = document;
    m_outline.clear();
    m_headingPositions.clear();
    m_anchors.clear();

    if (document) {
        QHash<QString, int> seen;
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            const int level = block.blockFormat().headingLevel();
            if (level > 0) {
                const QString title = block.text().simplified();
                // A repeated heading gets -1, -2, … after its id, as on GitHub.
                const QString base = slug(title);
                const int count = seen[base]++;
                const QString id = count == 0 ? base : QStringLiteral("%1-%2").arg(base).arg(count);
                if (!m_anchors.contains(id))
                    m_anchors.insert(id, block.position());
                m_headingPositions.append(block.position());
                m_outline.append(QVariantMap{{"title", title}, {"level", level}, {"position", block.position()}});
            }
            // Named anchors: footnotes, and any <a name> in raw HTML.
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextCharFormat format = it.fragment().charFormat();
                if (!format.isAnchor())
                    continue;
                for (const auto &name : format.anchorNames())
                    if (!m_anchors.contains(name))
                        m_anchors.insert(name, it.fragment().position());
            }
        }
    }
    emit outlineChanged();

    // The search belongs to the old layout; run it again on the new one.
    const int previous = m_currentMatch;
    findMatches();
    m_currentMatch = m_matches.isEmpty() ? -1 : qBound(0, previous, int(m_matches.size()) - 1);
    emit matchesChanged();

    if (!m_pendingAnchor.isEmpty()) {
        const QString anchor = m_pendingAnchor;
        m_pendingAnchor.clear();
        activateLink("#" + anchor);
    }
}

int Backend::headingAt(int position) const {
    const auto it = std::upper_bound(m_headingPositions.begin(), m_headingPositions.end(), position);
    return int(it - m_headingPositions.begin()) - 1;
}

void Backend::activateLink(const QString &link) {
    if (link.startsWith('#')) {
        const QString name = QUrl::fromPercentEncoding(link.mid(1).toUtf8());
        auto it = m_anchors.constFind(name);
        if (it == m_anchors.constEnd())
            it = m_anchors.constFind(name.toLower());
        if (it != m_anchors.constEnd())
            emit scrollToPosition(it.value());
        else if (name.isEmpty())
            emit scrollToPosition(0);
        else
            emit notice(tr("No heading named “%1” in this document.").arg(name));
        return;
    }

    const QUrl url(link);
    // A one-letter scheme is a Windows drive, not a protocol.
    if (url.scheme().size() > 1 && !url.isLocalFile()) {
        QDesktopServices::openUrl(url);
        return;
    }

    QString path = url.isLocalFile() ? url.toLocalFile() : url.path();
    if (path.isEmpty())
        path = link;
    if (QDir::isRelativePath(path))
        path = QDir(QFileInfo(m_filePath).absolutePath()).filePath(path);
    path = QDir::cleanPath(path);
    // Wiki links and many docs leave the extension off.
    if (!QFileInfo::exists(path) && QFileInfo::exists(path + ".md"))
        path += ".md";

    const QFileInfo info(path);
    if (!info.exists()) {
        emit notice(tr("Link target not found: %1").arg(path));
    } else if (info.isFile() && looksLikeMarkdown(path)) {
        const QString anchor = url.fragment();
        if (path == m_filePath) {
            if (!anchor.isEmpty())
                activateLink("#" + anchor);
            return;
        }
        // The page indexes itself as soon as it has the new file, so the
        // anchor to jump to has to be waiting before the file is opened.
        m_pendingAnchor = anchor;
        open(path);
        if (m_filePath != path)
            m_pendingAnchor.clear();
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void Backend::findMatches() {
    m_matches.clear();
    if (!m_document || m_query.isEmpty())
        return;
    QTextCursor cursor(m_document);
    while (m_matches.size() < MaxMatches) {
        cursor = m_document->find(m_query, cursor);
        if (cursor.isNull())
            break;
        m_matches.append({cursor.selectionStart(), cursor.selectionEnd()});
    }
}

void Backend::search(const QString &query, int fromPosition) {
    m_query = query;
    findMatches();
    m_currentMatch = -1;
    // Start from the first match at or below where the reader is looking.
    for (int i = 0; i < m_matches.size(); ++i) {
        if (m_matches[i].first >= fromPosition) {
            m_currentMatch = i;
            break;
        }
    }
    if (m_currentMatch < 0 && !m_matches.isEmpty())
        m_currentMatch = 0;
    emit matchesChanged();
    if (m_currentMatch >= 0)
        emit matchSelected(m_matches[m_currentMatch].first, m_matches[m_currentMatch].second);
}

void Backend::selectMatch(int index) {
    if (m_matches.isEmpty())
        return;
    const int count = m_matches.size();
    m_currentMatch = ((index % count) + count) % count;
    emit matchesChanged();
    emit matchSelected(m_matches[m_currentMatch].first, m_matches[m_currentMatch].second);
}

void Backend::nextMatch() { selectMatch(m_currentMatch + 1); }
void Backend::previousMatch() { selectMatch(m_currentMatch - 1); }
