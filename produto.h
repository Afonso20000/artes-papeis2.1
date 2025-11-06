#ifndef PRODUTO_H
#define PRODUTO_H

#include <QString>
#include <QColor>
#include <QDateTime>
#include <QVector>

// Order item representing a single product in an order
struct OrderItem {
    QString productId;
    QString productName;
    int quantity;
    double price;        // Price per unit
    double total() const { return quantity * price; }
};

// Message in order chat
struct ChatMessage {
    QString userId;
    QString userName;
    QString message;
    QDateTime timestamp;
};

// Complete order with client info and items
struct Order {
    QString orderId;
    QString userId;       // For user identification
    QString userName;     // For display
    QDateTime orderDate; // When the order was created
    QVector<OrderItem> items;
    double total;
    QString status;      // "pending", "accepted", "rejected"
    QVector<ChatMessage> chat; // Chat messages
    QDateTime lastUpdated; // Last status change or chat message
};

// Produto básico (usado no carrinho)
struct Produto {
    QString nome;
    double preco;
};

// Produto completo com cor para apresentação na loja
struct ProdutoFull {
    QString nome;
    double preco;
    QColor cor;
    QString id;
    int quantidade = 1;
    QString imagePath;    // caminho relativo para imagem armazenada (opcional)
    QString categoria;
    int lowThreshold = 2; // alerta de estoque baixo
};

#endif // PRODUTO_H