#include "portalfilepicker.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QRandomGenerator>
#include <QUrl>

namespace {

const char *Service = "org.freedesktop.portal.Desktop";
const char *RequestInterface = "org.freedesktop.portal.Request";

} // namespace

// The portal describes file filters as a(sa(us)): a name, then rules that
// are each a kind (0 for a glob, 1 for a MIME type) and a pattern.
struct PortalFilterRule {
    uint kind = 0;
    QString pattern;
};
struct PortalFilter {
    QString name;
    QList<PortalFilterRule> rules;
};
Q_DECLARE_METATYPE(PortalFilterRule)
Q_DECLARE_METATYPE(PortalFilter)

QDBusArgument &operator<<(QDBusArgument &argument, const PortalFilterRule &rule) {
    argument.beginStructure();
    argument << rule.kind << rule.pattern;
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFilterRule &rule) {
    argument.beginStructure();
    argument >> rule.kind >> rule.pattern;
    argument.endStructure();
    return argument;
}
QDBusArgument &operator<<(QDBusArgument &argument, const PortalFilter &filter) {
    argument.beginStructure();
    argument << filter.name << filter.rules;
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFilter &filter) {
    argument.beginStructure();
    argument >> filter.name >> filter.rules;
    argument.endStructure();
    return argument;
}

PortalFilePicker::PortalFilePicker(QObject *parent) : FilePicker(parent) {
    qDBusRegisterMetaType<PortalFilterRule>();
    qDBusRegisterMetaType<QList<PortalFilterRule>>();
    qDBusRegisterMetaType<PortalFilter>();
    qDBusRegisterMetaType<QList<PortalFilter>>();
}

void PortalFilePicker::forget() {
    if (m_requestPath.isEmpty())
        return;
    QDBusConnection::sessionBus().disconnect(Service, m_requestPath, RequestInterface, "Response", this,
                                             SLOT(onResponse(uint, QVariantMap)));
    m_requestPath.clear();
}

void PortalFilePicker::pick(const QString &title, const QString &directory) {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit failed(tr("The desktop file chooser is not available (no session bus)."));
        return;
    }
    forget();

    // The portal answers on a request object whose path is made from our bus
    // name and a token we choose, so we can listen before making the call
    // and never miss a fast answer.
    const QString token = QStringLiteral("mbingjustread%1").arg(QRandomGenerator::global()->generate());
    const QString sender = bus.baseService().mid(1).replace('.', '_');
    m_requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
    bus.connect(Service, m_requestPath, RequestInterface, "Response", this, SLOT(onResponse(uint, QVariantMap)));

    const QList<PortalFilter> filters{
        {tr("Markdown"), {{0, "*.md"}, {0, "*.markdown"}, {0, "*.mdown"}, {0, "*.mkd"}, {0, "*.MD"}, {1, "text/markdown"}}},
        {tr("Text"), {{1, "text/plain"}}},
        {tr("All files"), {{0, "*"}}},
    };
    QVariantMap options{
        {"handle_token", token},
        {"modal", true},
        {"filters", QVariant::fromValue(filters)},
    };
    // The portal wants the folder as a null-terminated byte string.
    if (!directory.isEmpty())
        options.insert("current_folder", directory.toLocal8Bit() + '\0');

    auto message = QDBusMessage::createMethodCall(Service, "/org/freedesktop/portal/desktop",
                                                  "org.freedesktop.portal.FileChooser", "OpenFile");
    message << QString() << title << options;
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();
        const QDBusPendingReply<QDBusObjectPath> reply = *call;
        if (!reply.isError())
            return;
        forget();
        emit failed(tr("The desktop file chooser could not be opened: %1").arg(reply.error().message()));
    });
}

void PortalFilePicker::onResponse(uint code, const QVariantMap &results) {
    forget();
    // Anything but 0 means the user closed the dialog, which is not an error.
    if (code != 0)
        return;
    const QStringList uris = results.value("uris").toStringList();
    if (!uris.isEmpty())
        emit picked(QUrl(uris.first()).toLocalFile());
}
