#include "doctor/domain/models.h"
#include "doctor/ui/main_window.h"

#include <QTest>

class SmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void statusTextIsStable() {
        QCOMPARE(
            QString::fromStdString(
                doctor::domain::toString(doctor::domain::TargetStatus::UnityFailed)),
            QStringLiteral("Unity Failed"));
    }

    void mainWindowCanBeConstructed() {
        doctor::ui::MainWindow window;
        QCOMPARE(window.windowTitle(), QStringLiteral("Unity Build Doctor"));
        QVERIFY(window.minimumWidth() >= 900);
    }
};

QTEST_MAIN(SmokeTest)
#include "smoke_test.moc"
