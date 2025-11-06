#include "productmanager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QMap>
#include <QList>

ProductManager::ProductManager(QObject* parent)
    : QObject(parent)
{
    produtos = loadProducts();
    // Also load order history
    loadOrders();
}

// Internal reservation map: product id -> list of (expireTime, qty)
static QMap<QString, QList<QPair<QDateTime,int>>> reservations;

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
        produtoObj["lowThreshold"] = produto.lowThreshold;
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
        produto.lowThreshold = obj.value("lowThreshold").toInt(2);
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

int ProductManager::getReservedCount(const QString& id) const {
    int total = 0;
    auto it = reservations.find(id);
    if (it == reservations.end()) return 0;
    QDateTime now = QDateTime::currentDateTime();
    for (const auto &p : it.value()) {
        if (p.first > now) total += p.second;
    }
    return total;
}

int ProductManager::getAvailableStock(const QString& id) const {
    ProdutoFull prod = getProduct(id);
    if (prod.id.isEmpty()) return 0;
    int reserved = getReservedCount(id);
    return qMax(0, prod.quantidade - reserved);
}

bool ProductManager::reserveProduct(const QString& id, int qty, const QDateTime& expires) {
    int avail = getAvailableStock(id);
    if (avail < qty) return false;
    reservations[id].append(qMakePair(expires, qty));
    return true;
}

void ProductManager::releaseExpiredReservations() {
    QDateTime now = QDateTime::currentDateTime();
    bool changed = false;
    for (auto it = reservations.begin(); it != reservations.end(); ) {
        auto &list = it.value();
        for (int i = list.size()-1; i >= 0; --i) {
            if (list[i].first <= now) {
                list.removeAt(i);
                changed = true;
            }
        }
    if (list.isEmpty()) it = reservations.erase(it); else ++it;
    }
    if (changed) emit productsChanged();
}

void ProductManager::saveOrder(const Order& order) {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists()) dataDir.mkpath(".");
    QString ordersPath = dataDir.filePath("orders.json");

    // Load existing orders
    QJsonArray ordersArray;
    QFile file(ordersPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        ordersArray = QJsonDocument::fromJson(file.readAll()).array();
        file.close();
    }

    // Add new order
    QJsonObject orderObj;
    orderObj["orderId"] = order.orderId;
    orderObj["userId"] = order.userId;
    orderObj["userName"] = order.userName;
    orderObj["orderDate"] = order.orderDate.toString(Qt::ISODate);
    orderObj["total"] = order.total;
    orderObj["status"] = order.status;
    orderObj["lastUpdated"] = order.lastUpdated.toString(Qt::ISODate);

    // Save items
    QJsonArray itemsArray;
    for (const OrderItem& item : order.items) {
        QJsonObject itemObj;
        itemObj["productId"] = item.productId;
        itemObj["productName"] = item.productName;
        itemObj["quantity"] = item.quantity;
        itemObj["price"] = item.price;
        itemsArray.append(itemObj);
    }
    orderObj["items"] = itemsArray;

    // Save chat messages
    QJsonArray chatArray;
    for (const ChatMessage& msg : order.chat) {
        QJsonObject msgObj;
        msgObj["userId"] = msg.userId;
        msgObj["userName"] = msg.userName;
        msgObj["message"] = msg.message;
        msgObj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
        chatArray.append(msgObj);
    }
    orderObj["chat"] = chatArray;

    ordersArray.append(orderObj);

    // Save updated orders
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(ordersArray).toJson());
        file.close();
    }
}

QVector<Order> ProductManager::loadOrders() const {
    QVector<Order> orders;
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString ordersPath = dataPath + "/artes-papeis/orders.json";

    QFile file(ordersPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return orders;

    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    file.close();

    for (const QJsonValue& val : array) {
        QJsonObject obj = val.toObject();
        Order order;
        order.orderId = obj["orderId"].toString();
        order.userId = obj["userId"].toString();
        order.userName = obj["userName"].toString();
        // If the saved order lacks a userName (older orders), try to read it from users.json
        if (order.userName.isEmpty()) {
            QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QString usersPath = QDir(dataPath + "/artes-papeis").filePath("users.json");
            QFile uf(usersPath);
            if (uf.open(QIODevice::ReadOnly)) {
                QJsonDocument udoc = QJsonDocument::fromJson(uf.readAll());
                if (udoc.isObject()) {
                    QJsonObject root = udoc.object();
                    if (root.contains(order.userId)) {
                        QJsonObject uobj = root.value(order.userId).toObject();
                        order.userName = uobj.value("fullName").toString();
                    }
                }
                uf.close();
            }
        }
        order.orderDate = QDateTime::fromString(obj["orderDate"].toString(), Qt::ISODate);
        order.total = obj["total"].toDouble();
        order.status = obj["status"].toString();
        if (order.status.isEmpty()) order.status = "pending";
        order.lastUpdated = QDateTime::fromString(obj["lastUpdated"].toString(), Qt::ISODate);
        if (!order.lastUpdated.isValid()) order.lastUpdated = order.orderDate;

        // Load items
        QJsonArray itemsArray = obj["items"].toArray();
        for (const QJsonValue& itemVal : itemsArray) {
            QJsonObject itemObj = itemVal.toObject();
            OrderItem item;
            item.productId = itemObj["productId"].toString();
            item.productName = itemObj["productName"].toString();
            item.quantity = itemObj["quantity"].toInt();
            item.price = itemObj["price"].toDouble();
            order.items.append(item);
        }

        // Load chat messages
        QJsonArray chatArray = obj["chat"].toArray();
        for (const QJsonValue& msgVal : chatArray) {
            QJsonObject msgObj = msgVal.toObject();
            ChatMessage msg;
            msg.userId = msgObj["userId"].toString();
            msg.userName = msgObj["userName"].toString();
            msg.message = msgObj["message"].toString();
            msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
            order.chat.append(msg);
        }

        orders.append(order);
    }
    return orders;
}

QVector<Order> ProductManager::getOrdersByClient(const QString& username) const {
    QVector<Order> allOrders = loadOrders();
    QVector<Order> clientOrders;
    for (const Order& order : allOrders) {
        if (order.userId == username)
            clientOrders.append(order);
    }
    return clientOrders;
}

QMap<QString, QPair<int, double>> ProductManager::getClientStats() const {
    QMap<QString, QPair<int, double>> stats;
    QVector<Order> allOrders = loadOrders();
    for (const Order& order : allOrders) {
        if (order.status == "accepted" || order.status == "pending") {
            auto &stat = stats[order.userId];
            stat.first++; // increment order count
            stat.second += order.total; // add to total spent
        }
    }
    return stats;
}

void ProductManager::commitProductSale(const QString& id, int qty) {
    for (int i = 0; i < produtos.size(); ++i) {
        if (produtos[i].id == id) {
            produtos[i].quantidade = qMax(0, produtos[i].quantidade - qty);
            saveProducts(produtos);
            emit productsChanged();
            break;
        }
    }
    // remove reservations starting from earliest until qty satisfied
    int remaining = qty;
    auto it = reservations.find(id);
    if (it != reservations.end()) {
        auto &list = it.value();
        // sort by expiry ascending (should already be in order)
        for (int i = 0; i < list.size() && remaining > 0; ++i) {
            int take = qMin(remaining, list[i].second);
            list[i].second -= take;
            remaining -= take;
        }
        for (int i = list.size()-1; i >= 0; --i) if (list[i].second <= 0) list.removeAt(i);
        if (list.isEmpty()) reservations.remove(id);
    }
}

QVector<ProdutoFull> ProductManager::getLowStockProducts() const {
    QVector<ProdutoFull> low;
    for (const auto &p : produtos) {
        int available = p.quantidade - getReservedCount(p.id);
        if (available <= p.lowThreshold) low.append(p);
    }
    return low;
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

ProdutoFull ProductManager::getProduct(const QString& id) const
{
    for (const auto& produto : produtos) {
        if (produto.id == id) {
            return produto;
        }
    }
    return ProdutoFull(); // retorna produto vazio se não encontrado
}

void ProductManager::releaseReservation(const QString& id, int qty) {
    if (qty <= 0) return;
    auto it = reservations.find(id);
    if (it == reservations.end()) return;
    int remaining = qty;
    // remove from the end (most recently reserved) first
    auto &list = it.value();
    for (int i = list.size()-1; i >= 0 && remaining > 0; --i) {
        int take = qMin(remaining, list[i].second);
        list[i].second -= take;
        remaining -= take;
    }
    for (int i = list.size()-1; i >= 0; --i) if (list[i].second <= 0) list.removeAt(i);
    if (list.isEmpty()) reservations.remove(id);
    emit productsChanged();
}