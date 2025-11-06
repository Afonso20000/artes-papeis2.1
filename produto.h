#ifndef PRODUTO_H
#define PRODUTO_H

#include <QString>
#include <QColor>

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