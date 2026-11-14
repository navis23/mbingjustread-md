#pragma once

#include <QVariantMap>

#include "filepicker.h"

// Opens the desktop's own file chooser through xdg-desktop-portal, so the
// dialog matches the rest of the system and works inside a sandbox.
class PortalFilePicker : public FilePicker {
    Q_OBJECT

public:
    explicit PortalFilePicker(QObject *parent = nullptr);

    void pick(const QString &title, const QString &directory) override;

private slots:
    void onResponse(uint code, const QVariantMap &results);

private:
    void forget();

    QString m_requestPath;
};
