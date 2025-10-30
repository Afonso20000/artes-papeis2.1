#ifndef PRODUCTCARD_H
#define PRODUCTCARD_H

#include <QWidget>
#include "produto.h"

class QVBoxLayout;
class QLabel;
class QPushButton;

class ProductCard : public QWidget
{
    Q_OBJECT

public:
    explicit ProductCard(const ProdutoFull& produto, QWidget* parent = nullptr);
    ~ProductCard();

signals:
    void compraProduto(const QString& id);

private:
    QVBoxLayout* layout;
    QLabel* imageLabel;
    QLabel* titleLabel;
    QLabel* priceLabel;
    QPushButton* buyButton;
};

#endif // PRODUCTCARD_H