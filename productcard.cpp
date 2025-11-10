#include "productcard.h"
#include "githubdarktheme.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

ProductCard::ProductCard(const ProdutoFull& produto, QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(240, 340);
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    imageLabel = new QLabel(this);
    imageLabel->setFixedSize(210, 140);
    if (!produto.imagePath.isEmpty()) {
        QPixmap px(produto.imagePath);
        if (!px.isNull()) {
            imageLabel->setPixmap(px.scaled(imageLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            imageLabel->setStyleSheet("border-radius: 8px;");
        } else {
            imageLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px; color: %2;")
                .arg(produto.cor.name())
                .arg(GitHubDark::TEXT_SECONDARY));
            imageLabel->setText("📷");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        imageLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px; color: %2; font-size: 32px;")
            .arg(GitHubDark::BG_OVERLAY)
            .arg(GitHubDark::TEXT_SECONDARY));
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setText("📷");
    }

    titleLabel = new QLabel(produto.nome, this);
    titleLabel->setStyleSheet(QString("font-size: 16px; font-weight: 600; color: %1; margin-top: 8px;")
        .arg(GitHubDark::TEXT_PRIMARY));
    titleLabel->setAlignment(Qt::AlignLeft);
    titleLabel->setWordWrap(true);

    priceLabel = new QLabel(QString::number(produto.preco, 'f', 2) + " €", this);
    priceLabel->setStyleSheet(QString("font-size: 18px; font-weight: 600; color: %1;")
        .arg(GitHubDark::ACCENT_PRIMARY));
    priceLabel->setAlignment(Qt::AlignLeft);

    stockBadge = new QLabel(this);
    stockBadge->setStyleSheet(QString("background: %1; color: %2; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 500;")
        .arg(GitHubDark::BG_TERTIARY)
        .arg(GitHubDark::TEXT_SECONDARY));
    stockBadge->setAlignment(Qt::AlignCenter);

    buyButton = new QPushButton("🛒 Adicionar", this);
    buyButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: 1px solid %1;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %2;
            border-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
        QPushButton:disabled {
            background-color: %4;
            border-color: %4;
            color: %5;
        }
    )").arg(GitHubDark::BUTTON_PRIMARY_BG)
       .arg(GitHubDark::BUTTON_PRIMARY_HOVER)
       .arg(GitHubDark::ACCENT_PRIMARY)
       .arg(GitHubDark::BG_TERTIARY)
       .arg(GitHubDark::TEXT_MUTED));

    connect(buyButton, &QPushButton::clicked, this, [this, id=produto.id]() {
        // Animação de feedback visual
        QString originalText = buyButton->text();
        buyButton->setText("✓ Adicionado!");
        buyButton->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: 1px solid %1;
                border-radius: 6px;
                padding: 8px 16px;
                font-weight: 600;
                font-size: 14px;
            }
        )").arg(GitHubDark::ACCENT_PRIMARY));
        
        // Criar animação de escala
        QPropertyAnimation* scaleAnim = new QPropertyAnimation(buyButton, "geometry");
        scaleAnim->setDuration(150);
        QRect originalGeometry = buyButton->geometry();
        QRect scaledGeometry = originalGeometry.adjusted(-5, -3, 5, 3);
        scaleAnim->setStartValue(originalGeometry);
        scaleAnim->setEndValue(scaledGeometry);
        scaleAnim->setEasingCurve(QEasingCurve::OutBack);
        
        // Voltar ao tamanho original
        QPropertyAnimation* scaleBackAnim = new QPropertyAnimation(buyButton, "geometry");
        scaleBackAnim->setDuration(150);
        scaleBackAnim->setStartValue(scaledGeometry);
        scaleBackAnim->setEndValue(originalGeometry);
        scaleBackAnim->setEasingCurve(QEasingCurve::InBack);
        
        connect(scaleAnim, &QPropertyAnimation::finished, [scaleBackAnim]() {
            scaleBackAnim->start(QAbstractAnimation::DeleteWhenStopped);
        });
        
        scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);
        
        // Restaurar texto e estilo após 800ms
        QTimer::singleShot(800, this, [this, originalText]() {
            buyButton->setText(originalText);
            buyButton->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    color: white;
                    border: 1px solid %1;
                    border-radius: 6px;
                    padding: 8px 16px;
                    font-weight: 600;
                    font-size: 14px;
                }
                QPushButton:hover {
                    background-color: %2;
                    border-color: %2;
                }
                QPushButton:pressed {
                    background-color: %3;
                }
                QPushButton:disabled {
                    background-color: %4;
                    border-color: %4;
                    color: %5;
                }
            )").arg(GitHubDark::BUTTON_PRIMARY_BG)
               .arg(GitHubDark::BUTTON_PRIMARY_HOVER)
               .arg(GitHubDark::ACCENT_PRIMARY)
               .arg(GitHubDark::BG_TERTIARY)
               .arg(GitHubDark::TEXT_MUTED));
        });
        
        emit compraProduto(id);
    });

    layout->addWidget(imageLabel, 0, Qt::AlignHCenter);
    layout->addWidget(titleLabel);
    layout->addWidget(priceLabel);
    layout->addWidget(stockBadge, 0, Qt::AlignLeft);
    layout->addStretch(1);
    layout->addWidget(buyButton);

    setLayout(layout);
    setStyleSheet(QString("background: %1; border-radius: 8px; border: 1px solid %2; padding: 0px;")
        .arg(GitHubDark::BG_SECONDARY)
        .arg(GitHubDark::BORDER_DEFAULT));
}

ProductCard::~ProductCard()
{}

void ProductCard::setAvailableStock(int available) {
    stockBadge->setText(QString("📦 Stock: %1").arg(available));
    if (available <= 0) {
        buyButton->setEnabled(false);
        stockBadge->setStyleSheet(QString("background: %1; color: white; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 500;")
            .arg(GitHubDark::ACCENT_RED));
    } else if (available <= 2) {
        stockBadge->setStyleSheet(QString("background: %1; color: #000; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 500;")
            .arg(GitHubDark::ACCENT_YELLOW));
        buyButton->setEnabled(true);
    } else {
        stockBadge->setStyleSheet(QString("background: %1; color: white; border-radius: 6px; padding: 4px 8px; font-size: 12px; font-weight: 500;")
            .arg(GitHubDark::ACCENT_PRIMARY));
        buyButton->setEnabled(true);
    }
}