// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cmath>

namespace {

// Used for any color the theme does not name, and for everything when there
// is no Omarchy theme at all.
const QHash<QString, QString> &defaults(bool dark) {
    static const QHash<QString, QString> darkColors{
        {"background", "#0e0e10"},        {"lighter_background", "#1b1b20"},
        {"foreground", "#c9c9d1"},        {"bright_foreground", "#f4f4f6"},
        {"dark_foreground", "#7f7f8a"},   {"muted", "#34343c"},
        {"selection", "#2f3b55"},         {"red", "#f7768e"},
        {"green", "#9ece6a"},             {"yellow", "#e0af68"},
        {"orange", "#ff9e64"},            {"blue", "#7aa2f7"},
        {"magenta", "#bb9af7"},           {"cyan", "#7dcfff"},
    };
    static const QHash<QString, QString> lightColors{
        {"background", "#fafafa"},        {"lighter_background", "#eeeef0"},
        {"foreground", "#2e2e33"},        {"bright_foreground", "#0e0e10"},
        {"dark_foreground", "#76767f"},   {"muted", "#d0d0d6"},
        {"selection", "#c9d8f5"},         {"red", "#c4314b"},
        {"green", "#3f7d20"},             {"yellow", "#8f6200"},
        {"orange", "#b15c00"},            {"blue", "#2a5db0"},
        {"magenta", "#7b3fc4"},           {"cyan", "#0f7b8a"},
    };
    return dark ? darkColors : lightColors;
}

} // namespace

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    // Without Hyprland the corners stay sharp, like Omarchy's default.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

// Every `name = "value"` line of colors.toml: the hex colors and the mode.
QHash<QString, QString> Theme::readColors(const QString &path) {
    QHash<QString, QString> colors;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return colors;
    static const QRegularExpression expression(R"re(^\s*([a-z_]+)\s*=\s*["'](#[0-9a-fA-F]{6}|dark|light)["'])re",
                                               QRegularExpression::MultilineOption);
    auto matches = expression.globalMatch(QString::fromUtf8(file.readAll()));
    while (matches.hasNext()) {
        const auto match = matches.next();
        colors.insert(match.captured(1), match.captured(2));
    }
    return colors;
}

QString Theme::readAccent(const QString &path) {
    const QString accent = readColors(path).value("accent");
    return accent.startsWith('#') ? accent : "#FFD60A";
}

QString Theme::color(const QString &name) const {
    const QString value = m_colors.value(name);
    return value.startsWith('#') ? value : defaults(dark()).value(name, "#808080");
}

// Black or white, whichever reads on the accent (WCAG relative luminance).
QString Theme::accentForeground() const {
    const QColor c(m_accent);
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
    return luminance > .179 ? "black" : "white";
}

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    // A theme switch replaces symlinks and files, so watch the parents too and
    // re-arm after every change. Watching the nearest existing ancestor means
    // installing Omarchy later works without restarting the app.
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const QString path = m_directory + "/theme/colors.toml";
    const auto colors = readColors(path);
    const auto accent = readAccent(path);
    if (accent != m_accent || colors != m_colors) {
        m_accent = accent;
        m_colors = colors;
        emit changed();
    }
}
