#include "doctor/ui/ui_theme.h"

#include <QColor>
#include <QPalette>

namespace doctor::ui {
namespace {

QColor mix(const QColor& first, const QColor& second, qreal amount) {
    return QColor::fromRgbF(
        first.redF() * (1.0 - amount) + second.redF() * amount,
        first.greenF() * (1.0 - amount) + second.greenF() * amount,
        first.blueF() * (1.0 - amount) + second.blueF() * amount,
        first.alphaF() * (1.0 - amount) + second.alphaF() * amount);
}

QString color(const QColor& value) {
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(value.red())
        .arg(value.green())
        .arg(value.blue())
        .arg(value.alpha());
}

}  // namespace

QString buildUiStyleSheet(const QPalette& palette) {
    const auto window = palette.color(QPalette::Window);
    const auto surface = palette.color(QPalette::Base);
    const auto text = palette.color(QPalette::Text);
    const bool dark = window.lightnessF() < 0.45;
    const auto muted = mix(text, window, dark ? 0.48 : 0.54);
    auto accent = palette.color(QPalette::Highlight);
    if (!dark && accent.lightnessF() > 0.58) {
        accent = mix(accent, QColor(Qt::black), 0.30);
    } else if (dark && accent.lightnessF() < 0.50) {
        accent = mix(accent, QColor(Qt::white), 0.22);
    }
    const auto accentText =
        accent.lightnessF() < 0.58 ? QColor(Qt::white) : QColor(Qt::black);
    const auto raised = mix(surface, text, dark ? 0.055 : 0.025);
    const auto input = mix(surface, window, dark ? 0.18 : 0.08);
    const auto border = mix(window, text, dark ? 0.24 : 0.15);
    const auto borderStrong = mix(window, text, dark ? 0.38 : 0.24);
    const auto hover = mix(surface, accent, dark ? 0.18 : 0.09);
    const auto selection = mix(surface, accent, dark ? 0.38 : 0.18);
    const auto accentHover = mix(accent, dark ? QColor(Qt::white) : QColor(Qt::black), 0.10);
    const auto danger = dark ? QColor(255, 120, 116) : QColor(190, 48, 44);
    const auto dangerSurface = mix(surface, danger, dark ? 0.18 : 0.09);

    auto style = QStringLiteral(R"QSS(
QWidget#setupPage, QWidget#workspacePage, QStackedWidget#mainPages {
    background: %1;
    color: %2;
}
QWidget {
    font-size: 14px;
}
QFrame[card="true"] {
    background: %3;
    border: 1px solid %4;
    border-radius: 14px;
}
QFrame[softCard="true"] {
    background: %5;
    border: 1px solid %4;
    border-radius: 11px;
}
QLabel[role="eyebrow"] {
    color: %6;
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel[role="title"] {
    color: %2;
    font-size: 30px;
    font-weight: 750;
}
QLabel[role="pageTitle"] {
    color: %2;
    font-size: 24px;
    font-weight: 700;
}
QLabel[role="cardTitle"] {
    color: %2;
    font-size: 17px;
    font-weight: 700;
}
QLabel[role="muted"] {
    color: %7;
}
QLabel[role="pill"] {
    color: %6;
    background: %8;
    border: 1px solid %9;
    border-radius: 12px;
    padding: 5px 10px;
    font-weight: 650;
}
QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextBrowser {
    color: %2;
    background: %10;
    border: 1px solid %4;
    border-radius: 8px;
    selection-background-color: %6;
    selection-color: %11;
}
QLineEdit, QComboBox, QSpinBox {
    min-height: 36px;
    padding: 0 10px;
}
QPlainTextEdit, QTextBrowser {
    padding: 8px 10px;
}
QLineEdit:hover, QComboBox:hover, QSpinBox:hover, QPlainTextEdit:hover {
    border-color: %12;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus,
QTextBrowser:focus {
    border: 2px solid %6;
}
QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled,
QPlainTextEdit:disabled {
    color: %7;
    background: %5;
    border-color: %4;
}
QPushButton {
    color: %2;
    background: %3;
    border: 1px solid %4;
    border-radius: 8px;
    min-height: 36px;
    padding: 0 15px;
    font-weight: 600;
}
QPushButton:hover {
    background: %13;
    border-color: %12;
}
QPushButton:pressed {
    background: %14;
}
QPushButton:disabled {
    color: %7;
    background: %5;
    border-color: %4;
}
QPushButton[role="primary"] {
    color: %11;
    background: %6;
    border-color: %6;
    min-height: 44px;
    padding: 0 22px;
    font-weight: 700;
}
QPushButton[role="primary"]:hover {
    background: %15;
    border-color: %15;
}
QPushButton[role="quiet"] {
    background: transparent;
    border-color: transparent;
}
QPushButton[role="danger"] {
    color: %16;
    background: %17;
    border-color: %18;
}
QProgressBar {
    color: %2;
    background: %5;
    border: 0;
    border-radius: 4px;
    min-height: 8px;
    max-height: 8px;
    text-align: center;
}
QProgressBar::chunk {
    background: %6;
    border-radius: 4px;
}
QTableWidget {
    color: %2;
    background: %3;
    alternate-background-color: %5;
    border: 0;
    gridline-color: %4;
    selection-background-color: %14;
    selection-color: %2;
}
QHeaderView::section {
    color: %7;
    background: %3;
    border: 0;
    border-bottom: 1px solid %4;
    padding: 10px 9px;
    font-size: 12px;
    font-weight: 700;
}
QTableCornerButton::section {
    background: %3;
    border: 0;
}
QSplitter::handle {
    background: %5;
    border-radius: 3px;
}
QSplitter::handle:horizontal {
    width: 10px;
    margin: 0 3px;
}
QSplitter::handle:vertical {
    height: 10px;
    margin: 3px 0;
}
QSplitter::handle:hover {
    background: %12;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: %12;
    border-radius: 4px;
    min-height: 28px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QToolTip {
    color: %2;
    background: %3;
    border: 1px solid %4;
    padding: 6px;
}
)QSS");
    style = style.arg(color(window));
    style = style.arg(color(text));
    style = style.arg(color(surface));
    style = style.arg(color(border));
    style = style.arg(color(raised));
    style = style.arg(color(accent));
    style = style.arg(color(muted));
    style = style.arg(color(selection));
    style = style.arg(color(mix(surface, accent, dark ? 0.45 : 0.28)));
    style = style.arg(color(input));
    style = style.arg(color(accentText));
    style = style.arg(color(borderStrong));
    style = style.arg(color(hover));
    style = style.arg(color(selection));
    style = style.arg(color(accentHover));
    style = style.arg(color(danger));
    style = style.arg(color(dangerSurface));
    return style.arg(color(mix(surface, danger, 0.35)));
}

}  // namespace doctor::ui
