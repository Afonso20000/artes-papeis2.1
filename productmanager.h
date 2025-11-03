#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <QObject>
#include <QVector>
#include <QDateTime>
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

    // Reservation & stock helpers
    int getReservedCount(const QString& id) const;
    int getAvailableStock(const QString& id) const;
    bool reserveProduct(const QString& id, int qty, const QDateTime& expires);
    void releaseReservation(const QString& id, int qty);
    void releaseExpiredReservations();
    void commitProductSale(const QString& id, int qty); // permanently reduce stock (checkout)
    QVector<ProdutoFull> getLowStockProducts() const;

private:
    QString generateId() const;

signals:
    void productsChanged();

private:
    QString getStoragePath() const;
    QVector<ProdutoFull> produtos;
};

#endif // PRODUCTMANAGER_H