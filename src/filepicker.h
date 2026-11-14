#pragma once

#include <QObject>
#include <QString>

// How the app asks the user for a file to read. Linux goes through the
// desktop portal; other platforms and the tests supply their own.
class FilePicker : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    virtual void pick(const QString &title, const QString &directory) = 0;

signals:
    void picked(const QString &path);
    void failed(const QString &reason);
};
