#include "ui/MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    // The offscreen plugin does not enumerate Windows system fonts automatically.
    if (QApplication::platformName() == "offscreen") {
        QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/msyh.ttc");
        QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/msyhbd.ttc");
    }
#endif
    app.setApplicationName("OS Algorithm Visualizer");
    app.setOrganizationName("OS Lab");
    app.setStyle("Fusion");
    MainWindow window;
    window.show();
    const auto args = app.arguments();
    if (args.contains("--smoke-test") || args.contains("--screenshots")) {
        QString directory;
        int at = args.indexOf("--screenshots");
        if (at >= 0 && at + 1 < args.size()) directory = args[at + 1];
        QTimer::singleShot(150, &window, [&, directory] {
            std::fprintf(stderr, "OSVisualizer: smoke test starting\n"); std::fflush(stderr);
            bool passed = window.smokeTest(directory);
            std::fprintf(stderr, "OSVisualizer: smoke test %s\n", passed ? "PASS" : "FAIL"); std::fflush(stderr);
            app.exit(passed ? 0 : 1);
        });
    }
    return app.exec();
}
