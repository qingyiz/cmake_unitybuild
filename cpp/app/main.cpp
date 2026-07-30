#include "doctor/ui/main_window.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("UnityBuildDoctor"));
    QCoreApplication::setApplicationName(QStringLiteral("Unity Build Doctor"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("以图形界面分析 CMake Unity Build 冲突"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption screenshotOption(
        QStringList{QStringLiteral("s"), QStringLiteral("screenshot")},
        QStringLiteral("启动后保存界面截图并退出。"),
        QStringLiteral("path"));
    parser.addOption(screenshotOption);
    parser.process(application);

    doctor::ui::MainWindow window;
    window.show();
    if (parser.isSet(screenshotOption)) {
        const auto path = parser.value(screenshotOption);
        QTimer::singleShot(300, &application, [&application, &window, path] {
            window.grab().save(path);
            application.quit();
        });
    }
    return application.exec();
}
