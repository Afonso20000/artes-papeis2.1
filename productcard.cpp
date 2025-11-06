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

    stockBadge = new QLabel(this);
    stockBadge->setStyleSheet("background: #fffb; color: #0E141C; border-radius: 8px; padding: 4px 8px; font-size: 12px;");
    stockBadge->setAlignment(Qt::AlignCenter);
    stockBadge->setFixedWidth(80);

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
    layout->addWidget(stockBadge, 0, Qt::AlignHCenter);
    layout->addWidget(priceLabel);
    layout->addStretch(1);
    layout->addWidget(buyButton, 0, Qt::AlignHCenter);

    setLayout(layout);
    setStyleSheet("background: #F4F4F4; border-radius: 14px; border: 1px solid #8197AC; padding: 11px;");
}

ProductCard::~ProductCard()
{}

void ProductCard::setAvailableStock(int available) {
    stockBadge->setText(QString("Stock: %1").arg(available));
    if (available <= 0) {
        buyButton->setEnabled(false);
        stockBadge->setStyleSheet("background: #ff6666; color: #ffffff; border-radius: 8px; padding: 4px 8px; font-size: 12px;");
    } else if (available <= 2) {
        stockBadge->setStyleSheet("background: #ffcc66; color: #000000; border-radius: 8px; padding: 4px 8px; font-size: 12px;");
        buyButton->setEnabled(true);
    } else {
        stockBadge->setStyleSheet("background: #bdf3b8; color: #000000; border-radius: 8px; padding: 4px 8px; font-size: 12px;");
        buyButton->setEnabled(true);
    }
}