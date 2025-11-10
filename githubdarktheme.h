#ifndef GITHUBDARKTHEME_H
#define GITHUBDARKTHEME_H

#include <QString>

namespace GitHubDark {
    // Cores principais do GitHub Dark Mode
    const QString BG_PRIMARY = "#0d1117";        // Fundo principal
    const QString BG_SECONDARY = "#161b22";      // Fundo secundário (cards, etc)
    const QString BG_TERTIARY = "#21262d";       // Fundo terciário (hover)
    const QString BG_OVERLAY = "#30363d";        // Overlay/dropdown
    
    const QString BORDER_DEFAULT = "#30363d";    // Bordas padrão
    const QString BORDER_MUTED = "#21262d";      // Bordas sutis
    
    const QString TEXT_PRIMARY = "#c9d1d9";      // Texto principal
    const QString TEXT_SECONDARY = "#8b949e";    // Texto secundário
    const QString TEXT_LINK = "#58a6ff";         // Links
    const QString TEXT_MUTED = "#6e7681";        // Texto desbotado
    
    const QString ACCENT_PRIMARY = "#238636";    // Verde (sucesso/confirmação)
    const QString ACCENT_EMPHASIS = "#2ea043";   // Verde hover
    const QString ACCENT_BLUE = "#1f6feb";       // Azul (informação)
    const QString ACCENT_PURPLE = "#8957e5";     // Roxo (processamento)
    const QString ACCENT_ORANGE = "#db6d28";     // Laranja (aviso)
    const QString ACCENT_RED = "#da3633";        // Vermelho (erro/recusa)
    const QString ACCENT_YELLOW = "#d29922";     // Amarelo (pendente)
    
    const QString BUTTON_PRIMARY_BG = "#238636";
    const QString BUTTON_PRIMARY_HOVER = "#2ea043";
    const QString BUTTON_SECONDARY_BG = "#21262d";
    const QString BUTTON_SECONDARY_HOVER = "#30363d";
    
    const QString INPUT_BG = "#0d1117";
    const QString INPUT_BORDER = "#30363d";
    const QString INPUT_FOCUS_BORDER = "#58a6ff";
    
    // Stylesheet global para a aplicação
    inline QString getGlobalStyleSheet() {
        return QString(R"(
            QMainWindow {
                background-color: %1;
                color: %2;
            }
            QWidget {
                background-color: %1;
                color: %2;
            }
            QLabel {
                color: %2;
                background-color: transparent;
            }
            QPushButton {
                background-color: %3;
                color: %2;
                border: 1px solid %4;
                padding: 8px 16px;
                border-radius: 6px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: %5;
                border-color: %6;
            }
            QPushButton:pressed {
                background-color: %7;
            }
            QLineEdit, QTextEdit {
                background-color: %8;
                color: %2;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 8px;
                selection-background-color: %9;
            }
            QLineEdit:focus, QTextEdit:focus {
                border-color: %10;
            }
            QScrollArea {
                background-color: %1;
                border: none;
            }
            QScrollBar:vertical {
                background-color: %1;
                width: 14px;
                border-radius: 7px;
            }
            QScrollBar::handle:vertical {
                background-color: %6;
                min-height: 30px;
                border-radius: 7px;
                margin: 2px;
            }
            QScrollBar::handle:vertical:hover {
                background-color: %5;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0;
            }
            QScrollBar:horizontal {
                background-color: %1;
                height: 14px;
                border-radius: 7px;
            }
            QScrollBar::handle:horizontal {
                background-color: %6;
                min-width: 30px;
                border-radius: 7px;
                margin: 2px;
            }
            QScrollBar::handle:horizontal:hover {
                background-color: %5;
            }
            QTableWidget {
                background-color: %1;
                alternate-background-color: %11;
                color: %2;
                gridline-color: %4;
                border: 1px solid %4;
                border-radius: 6px;
            }
            QTableWidget::item {
                padding: 8px;
                border-bottom: 1px solid %12;
            }
            QTableWidget::item:selected {
                background-color: %13;
                color: %2;
            }
            QHeaderView::section {
                background-color: %11;
                color: %2;
                padding: 8px;
                border: none;
                border-bottom: 1px solid %4;
                font-weight: bold;
            }
            QListWidget {
                background-color: %1;
                border: 1px solid %4;
                border-radius: 6px;
                color: %2;
            }
            QListWidget::item {
                padding: 12px;
                border-bottom: 1px solid %12;
            }
            QListWidget::item:hover {
                background-color: %5;
            }
            QListWidget::item:selected {
                background-color: %13;
                color: %2;
            }
            QComboBox {
                background-color: %8;
                color: %2;
                border: 1px solid %4;
                border-radius: 6px;
                padding: 8px;
                min-width: 150px;
            }
            QComboBox:hover {
                border-color: %6;
            }
            QComboBox::drop-down {
                border: none;
                padding-right: 8px;
            }
            QComboBox::down-arrow {
                image: none;
                border-left: 5px solid transparent;
                border-right: 5px solid transparent;
                border-top: 5px solid %2;
                width: 0;
                height: 0;
            }
            QComboBox QAbstractItemView {
                background-color: %14;
                border: 1px solid %4;
                border-radius: 6px;
                selection-background-color: %13;
                color: %2;
                padding: 4px;
            }
            QTabWidget::pane {
                border: 1px solid %4;
                background-color: %1;
                border-radius: 6px;
                padding: 16px;
            }
            QTabBar::tab {
                background-color: %11;
                color: %15;
                padding: 10px 20px;
                margin-right: 4px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
            }
            QTabBar::tab:hover {
                background-color: %5;
                color: %2;
            }
            QTabBar::tab:selected {
                background-color: %16;
                color: %2;
                font-weight: bold;
            }
            QGroupBox {
                color: %2;
                border: 1px solid %4;
                border-radius: 6px;
                margin-top: 12px;
                padding: 16px;
                background-color: %11;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 8px;
                color: %17;
                font-weight: bold;
            }
            QMessageBox {
                background-color: %11;
                color: %2;
            }
            QMessageBox QLabel {
                color: %2;
            }
            QDialog {
                background-color: %11;
                color: %2;
            }
        )").arg(
            BG_PRIMARY,           // 1
            TEXT_PRIMARY,         // 2
            BUTTON_SECONDARY_BG,  // 3
            BORDER_DEFAULT,       // 4
            BG_TERTIARY,          // 5
            TEXT_SECONDARY,       // 6
            BORDER_MUTED,         // 7
            INPUT_BG,             // 8
            ACCENT_BLUE,          // 9
            INPUT_FOCUS_BORDER,   // 10
            BG_SECONDARY,         // 11
            BORDER_MUTED,         // 12
            BG_TERTIARY,          // 13
            BG_OVERLAY,           // 14
            TEXT_SECONDARY,       // 15
            ACCENT_BLUE,          // 16
            ACCENT_BLUE           // 17
        );
    }
}

#endif // GITHUBDARKTHEME_H
