#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "produto.h"
#include "productmanager.h"

class QStackedWidget;
class QLabel;
class QWidget;
class QPushButton;
class QHBoxLayout;
class QGridLayout;
class QScrollArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void abrirBlog();
    void abrirSobre();
    void abrirLoja();
    void abrirInicio();
    void abrirContato();
    void adicionarAoCarrinho(const QString& nome, double preco);
    void mostrarCarrinho();

    // Admin
    void solicitarAdmin();
    void tentarLoginAdmin(const QString& senha);
    void logoutAdmin();

private:
    void atualizarCarrinhoIcon();
    void atualizarCarrinhoPagina();

    QVector<Produto> carrinho;
    QLabel* carrinhoIconLabel;
    QStackedWidget* paginas;
    QWidget* lojaPage;
    QWidget* carrinhoPage;
    QWidget* blogPage;
    QWidget* sobrePage;
    QWidget* inicioPage;
    QWidget* contatoPage;

    // Loja: produtos geridos por código, mutáveis via admin
    ProductManager* productManager;
    QWidget* productsWidget = nullptr;
    QGridLayout* productsGrid = nullptr;
    QScrollArea* productsScroll = nullptr;
    QPushButton* editProductsButton = nullptr; // visível apenas para admin

        // Gerir produtos (apenas admin)
    void refreshLojaProducts();
    void showProductManager();
    void updateAdminUI();

    // Admin page and state
    QWidget* adminPage;
    bool isAdmin = false;
    QPushButton* adminButton = nullptr;
};

#endif // MAINWINDOW_H