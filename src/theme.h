#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QTimer>

// The reader's whole palette comes from the Omarchy theme, so a page looks
// like the rest of the desktop. Without Omarchy it falls back to a neutral
// dark palette, which is also what the macOS and Windows builds start from.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString surface READ surface NOTIFY changed)
    Q_PROPERTY(QString foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QString brightForeground READ brightForeground NOTIFY changed)
    Q_PROPERTY(QString mutedForeground READ mutedForeground NOTIFY changed)
    Q_PROPERTY(QString border READ border NOTIFY changed)
    Q_PROPERTY(QString selection READ selection NOTIFY changed)
    Q_PROPERTY(QString red READ red NOTIFY changed)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr, const QString &hyprctl = {});
    ~Theme() override;

    QString accent() const { return m_accent; }
    QString accentForeground() const;
    int radius() const { return m_radius; }
    bool dark() const { return m_colors.value("mode", "dark") != "light"; }

    QString background() const { return color("background"); }
    QString surface() const { return color("lighter_background"); }
    QString foreground() const { return color("foreground"); }
    QString brightForeground() const { return color("bright_foreground"); }
    QString mutedForeground() const { return color("dark_foreground"); }
    QString border() const { return color("muted"); }
    QString selection() const { return color("selection"); }
    QString red() const { return color("red"); }

    // Any key of colors.toml, or the built-in default for it.
    QString color(const QString &name) const;

    static QString readAccent(const QString &path);
    static QHash<QString, QString> readColors(const QString &path);

signals:
    void changed();

private:
    void reload();
    void refreshRounding();

    QString m_directory;
    QString m_accent = "#FFD60A";
    QHash<QString, QString> m_colors;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
