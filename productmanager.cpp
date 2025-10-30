#include "productmanager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

ProductManager::ProductManager(QObject* parent)
    : QObject(parent)
{
    produtos = loadProducts();
}

QString ProductManager::getStoragePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    return dataDir.filePath("produtos.json");
}

bool ProductManager::saveProducts(const QVector<ProdutoFull>& produtos)
{
    QJsonArray jsonArray;
    for (const auto& produto : produtos) {
        QJsonObject produtoObj;
        produtoObj["nome"] = produto.nome;
        produtoObj["preco"] = produto.preco;
        produtoObj["cor"] = produto.cor.name();
        produtoObj["id"] = produto.id;
        produtoObj["quantidade"] = produto.quantidade;
        produtoObj["imagePath"] = produto.imagePath;
        produtoObj["categoria"] = produto.categoria;
        jsonArray.append(produtoObj);
    }

    QFile file(getStoragePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    return true;
}

QVector<ProdutoFull> ProductManager::loadProducts()
{
    QVector<ProdutoFull> loadedProducts;
    QFile file(getStoragePath());
    
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return loadedProducts;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray jsonArray = doc.array();

    for (const QJsonValue& value : jsonArray) {
        QJsonObject obj = value.toObject();
        ProdutoFull produto;
        produto.nome = obj["nome"].toString();
        produto.preco = obj["preco"].toDouble();
        produto.cor = QColor(obj["cor"].toString());
        produto.id = obj["id"].toString();
        produto.quantidade = obj["quantidade"].toInt();
        produto.imagePath = obj["imagePath"].toString();
        produto.categoria = obj["categoria"].toString();
        loadedProducts.append(produto);
    }

    return loadedProducts;
}

void ProductManager::addProduct(const ProdutoFull& produto)
{
    produtos.append(produto);
    saveProducts(produtos);
    emit productsChanged();
}

void ProductManager::updateProduct(const QString& id, const ProdutoFull& produto)
{
    for (int i = 0; i < produtos.size(); ++i) {
        if (produtos[i].id == id) {
            produtos[i] = produto;
            saveProducts(produtos);
            emit productsChanged();
            return;
        }
    }
}

void ProductManager::removeProduct(const QString& id)
{
    for (int i = 0; i < produtos.size(); ++i) {
        if (produtos[i].id == id) {
            produtos.remove(i);
            saveProducts(produtos);
            emit productsChanged();
            return;
        }
    }
}

ProdutoFull* ProductManager::findProductById(const QString& id)
{
    for (auto& produto : produtos) {
        if (produto.id == id) {
            return &produto;
        }
    }
    return nullptr;
}

QVector<ProdutoFull> ProductManager::getAllProducts() const
{
    return produtos;
}