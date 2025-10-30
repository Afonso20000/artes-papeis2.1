#include "productcard.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

ProductCard::ProductCard(const ProdutoFull& produto, QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(210, 320);
    layout = new QVBoxLayout(this);

    imageLabel = new QLabel(this);
    imageLabel->setFixedSize(175, 110);
    if (!produto.imagePath.isEmpty()) {
        QPixmap px(produto.imagePath);
        if (!px.isNull()) {
            imageLabel->setPixmap(px.scaled(imageLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            imageLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(produto.cor.name()));
            imageLabel->setText("Imagem");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        imageLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(produto.cor.name()));
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setText("Imagem");
    }

    titleLabel = new QLabel(produto.nome, this);
    titleLabel->setStyleSheet("font-size: 17px; font-weight: bold; color: #0E141C; margin-top: 10px;");
    titleLabel->setAlignment(Qt::AlignHCenter);

    priceLabel = new QLabel(QString::number(produto.preco, 'f', 2) + " €", this);
    priceLabel->setStyleSheet("font-size: 16px; color: #314B6E;");
    priceLabel->setAlignment(Qt::AlignHCenter);

    buyButton = new QPushButton("Comprar", this);
    buyButton->setStyleSheet(
        "background-color: #BDB3A3; color: #0E141C; "
        "border-radius: 8px; padding: 6px 15px; font-weight: bold;"
    );

    connect(buyButton, &QPushButton::clicked, this, [this, id=produto.id]() {
        emit compraProduto(id);
    });

    layout->addWidget(imageLabel, 0, Qt::AlignHCenter);
    layout->addWidget(titleLabel);
    layout->addWidget(priceLabel);
    layout->addStretch(1);
    layout->addWidget(buyButton, 0, Qt::AlignHCenter);

    setLayout(layout);
    setStyleSheet("background: #F4F4F4; border-radius: 14px; border: 1px solid #8197AC; padding: 11px;");
}

ProductCard::~ProductCard()
{}