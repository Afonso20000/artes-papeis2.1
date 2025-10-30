#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <QObject>
#include <QVector>
#include "produto.h"

class ProductManager : public QObject
{
    Q_OBJECT

public:
    explicit ProductManager(QObject* parent = nullptr);

    bool saveProducts(const QVector<ProdutoFull>& produtos);
    QVector<ProdutoFull> loadProducts();
    void addProduct(const ProdutoFull& produto);
    void updateProduct(const QString& id, const ProdutoFull& produto);
    void removeProduct(const QString& id);
    ProdutoFull* findProductById(const QString& id);
    QVector<ProdutoFull> getAllProducts() const { return produtos; }
    ProdutoFull getProduct(const QString& id) const;

private:
    QString generateId() const;

signals:
    void productsChanged();

private:
    QString getStoragePath() const;
    QVector<ProdutoFull> produtos;
};

#endif // PRODUCTMANAGER_H