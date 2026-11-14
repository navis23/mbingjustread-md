#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class FilePicker;
class QQuickTextDocument;
class Theme;

// The reader: which file is open, its rendered page, its outline, and the
// search through it. It never writes to the file it shows.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString html READ html NOTIFY loaded)
    Q_PROPERTY(QString filePath READ filePath NOTIFY fileChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileChanged)
    Q_PROPERTY(QUrl baseUrl READ baseUrl NOTIFY fileChanged)
    Q_PROPERTY(bool hasDocument READ hasDocument NOTIFY fileChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantList outline READ outline NOTIFY outlineChanged)
    Q_PROPERTY(int fontSize READ fontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(int contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY matchesChanged)
    Q_PROPERTY(int currentMatch READ currentMatch NOTIFY matchesChanged)

public:
    // `persist` keeps the zoom level between runs; tests turn it off.
    explicit Backend(Theme *theme, FilePicker *picker = nullptr, QObject *parent = nullptr, bool persist = true);

    QString html() const { return m_html; }
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    QUrl baseUrl() const;
    bool hasDocument() const { return !m_filePath.isEmpty(); }
    QString error() const { return m_error; }
    QVariantList outline() const { return m_outline; }
    int fontSize() const { return m_fontSize; }
    int contentWidth() const { return m_contentWidth; }
    void setContentWidth(int width);
    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex + 1 < m_history.size(); }
    int matchCount() const { return m_matches.size(); }
    int currentMatch() const { return m_currentMatch; }

    // Takes a path or a file: URL, as given on the command line or dropped.
    Q_INVOKABLE void open(const QString &pathOrUrl);
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void back();
    Q_INVOKABLE void forward();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void zoomReset();

    // A link in the page was clicked: jump within the page, open another
    // Markdown file here, or hand anything else to the desktop.
    Q_INVOKABLE void activateLink(const QString &link);

    // Called by the page once it has laid out new HTML, so the outline, the
    // anchors, and the search can be read from what is really on screen.
    Q_INVOKABLE void attachDocument(QQuickTextDocument *document);
    void indexDocument(QTextDocument *document);

    Q_INVOKABLE void search(const QString &query, int fromPosition = 0);
    Q_INVOKABLE void nextMatch();
    Q_INVOKABLE void previousMatch();
    // Index of the last heading at or before a position in the page.
    Q_INVOKABLE int headingAt(int position) const;

    // The id GitHub gives a heading, so `#like-this` links work the same.
    static QString slug(const QString &title);

signals:
    // New HTML is ready. `sameFile` is true when it is the file already on
    // screen (a reload, zoom, or theme change), so the page keeps its place.
    void loaded(bool sameFile);
    void fileChanged();
    void errorChanged();
    void outlineChanged();
    void fontSizeChanged();
    void contentWidthChanged();
    void historyChanged();
    void matchesChanged();
    void scrollToPosition(int position);
    void matchSelected(int start, int end);
    void notice(const QString &text);

private:
    bool load(const QString &path, bool sameFile);
    void render(bool sameFile);
    void setError(const QString &error);
    void setFontSize(int size);
    void watch();
    void findMatches();
    void selectMatch(int index);

    Theme *m_theme;
    FilePicker *m_picker;
    bool m_persist;

    QString m_filePath, m_source, m_html, m_error;
    QStringList m_history;
    int m_historyIndex = -1;
    int m_fontSize = 16;
    int m_contentWidth = 760;

    QFileSystemWatcher m_watcher;
    QTimer m_reloadDebounce, m_widthDebounce;

    QPointer<QTextDocument> m_document;
    QVariantList m_outline;
    QList<int> m_headingPositions;
    QHash<QString, int> m_anchors;
    QString m_pendingAnchor;

    QString m_query;
    QList<QPair<int, int>> m_matches;
    int m_currentMatch = -1;
};
