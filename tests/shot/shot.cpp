// Drives the real Main.qml without a display and saves what the window
// shows, so a change can be looked at without touching the desktop.
//
//   shot <themeDir|-> <file.md|-> <out.png> [step ...]
//
// "-" as the theme directory follows the Omarchy theme and language of this
// machine. Any other directory is read instead and may hold colors.toml and
// a "language" file; an empty one gives the built-in palette and English,
// which is what a screenshot for the README wants. Steps run in order:
//   T  ?  J  Shift+G  Ctrl+F ...   press that key
//   find:text                      open the search bar and type
//   open:/path/file.md             open another file (prints the time taken)
//   link:other.md#section          follow a link
//   click:x,y                      left click
//   w:900  h:700                   resize the window
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTest>
#include <QTimer>

#include "backend.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Material");

    const QStringList arguments = app.arguments();
    if (arguments.size() < 4) {
        qWarning("usage: shot <themeDir|-> <file.md|-> <out.png> [step ...]");
        return 2;
    }
    const QString themeDirectory = arguments[1] == "-" ? QString() : arguments[1];

    Translator translator(themeDirectory.isEmpty() ? QString() : themeDirectory + "/language");
    app.installTranslator(&translator);
    Theme theme(themeDirectory);
    // No file picker and no saved settings: a run must not depend on, or
    // change, the user's own zoom level.
    Backend backend(&theme, nullptr, nullptr, false);
    if (arguments[2] != "-")
        backend.open(arguments[2]);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());

    QTimer::singleShot(400, [&] {
        for (int i = 4; i < arguments.size(); ++i) {
            const QString step = arguments[i];
            QElapsedTimer timer;
            timer.start();
            if (step.startsWith("w:")) {
                window->setWidth(step.mid(2).toInt());
            } else if (step.startsWith("h:")) {
                window->setHeight(step.mid(2).toInt());
            } else if (step.startsWith("find:")) {
                QTest::keyClick(window, Qt::Key_Slash);
                QTest::qWait(100);
                for (const QChar character : step.mid(5))
                    QTest::keyClick(window, character.toLatin1());
            } else if (step.startsWith("open:")) {
                backend.open(step.mid(5));
                QTest::qWait(1);
                qWarning() << step << "took" << timer.elapsed() << "ms";
            } else if (step.startsWith("link:")) {
                backend.activateLink(step.mid(5));
            } else if (step.startsWith("click:")) {
                const QStringList point = step.mid(6).split(',');
                QTest::mouseClick(window, Qt::LeftButton, {},
                                  QPoint(point.value(0).toInt(), point.value(1).toInt()));
            } else {
                const QKeySequence sequence(step);
                QTest::keyClick(window, sequence[0].key(), sequence[0].keyboardModifiers());
                QTest::qWait(1);
                qWarning() << step << "took" << timer.elapsed() << "ms";
            }
            QTest::qWait(250);
        }
        QTest::qWait(300);
        const bool saved = window->grabWindow().save(arguments[3]);
        if (!saved)
            qWarning() << "could not write" << arguments[3];
        app.exit(saved ? 0 : 1);
    });
    return app.exec();
}
