// mbingjustread-md — a fast, accurate reader for Markdown files. It only reads.

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include <cstdio>
#include <cstring>

#include "backend.h"
#include "portalfilepicker.h"
#include "theme.h"
#include "translator.h"

static const char *Version = "0.1.0";

int main(int argc, char *argv[]) {
    // Answered before the GUI starts, so they work without a display.
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--version") || !std::strcmp(argv[i], "-v")) {
            std::printf("mbingjustread-md %s\n", Version);
            return 0;
        }
        if (!std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h")) {
            std::printf("Usage: mbingjustread-md [file.md]\n\n"
                        "Reads a Markdown file. Press ? in the window for the shortcuts.\n");
            return 0;
        }
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("mbingjustread-md");
    app.setOrganizationName("mbingjustread-md");
    app.setApplicationVersion(Version);

    // Matches the window to mbingjustread-md.desktop, so the compositor
    // (Wayland app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName("mbingjustread-md");
    app.setWindowIcon(QIcon::fromTheme("mbingjustread-md"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    PortalFilePicker picker;
    Backend backend(&theme, &picker);

    const QStringList arguments = app.arguments();
    if (arguments.size() > 1)
        backend.open(arguments.last());

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
