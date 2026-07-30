#pragma once

#include <QWidget>
#include <QVariantMap>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QFrame;
class QSplitter;
class QTableWidget;
class QTextBrowser;

namespace doctor::ui {

class AnalysisWorkspaceWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisWorkspaceWidget(QWidget* parent = nullptr);

    void reset(const QString& project, bool sourceScan = false);
    void updateProgress(
        const QString& stage,
        const QString& target,
        const QString& message,
        int completed,
        int total);
    void appendLog(const QString& text);
    void addTarget(const QVariantMap& target);
    void setFinished(bool cancelled);

signals:
    void cancelRequested();
    void backRequested();
    void exportRequested();

private:
    void showTargetDetails(int row);
    void applyFilter(const QString& text);
    void setDetailFocus(bool focused);

    QLabel* projectLabel_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* summaryLabel_{nullptr};
    QLabel* progressTitle_{nullptr};
    QLabel* resultsTitle_{nullptr};
    QProgressBar* progress_{nullptr};
    QPushButton* cancelButton_{nullptr};
    QPushButton* exportButton_{nullptr};
    QPushButton* focusDetailsButton_{nullptr};
    QLineEdit* filterEdit_{nullptr};
    QFrame* targetsPane_{nullptr};
    QFrame* logCard_{nullptr};
    QSplitter* horizontalSplitter_{nullptr};
    QSplitter* detailSplitter_{nullptr};
    QTableWidget* targetsTable_{nullptr};
    QTextBrowser* details_{nullptr};
    QPlainTextEdit* cmake_{nullptr};
    QPlainTextEdit* logs_{nullptr};
    bool sourceScan_{false};
    bool detailFocused_{false};
};

}  // namespace doctor::ui
