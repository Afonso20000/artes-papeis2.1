#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Qt Widgets
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>

// Qt Core
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QVector>

// Local includes
#include "produto.h"
#include "productmanager.h"
#include "clickablelabel.h"

// Estrutura para representar uma ordem
struct OrderItem {
    QString productId;
    QString productName;
    int quantity;
    double price;
};

struct Order {
    QString orderId;
    QString userId;
    QString userName;
    QDateTime orderDate;
    QVector<OrderItem> items;
    double total;
};

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    void abrirSobre();
    void abrirLoja();
    void abrirInicio();
    void abrirContato();
    void adicionarAoCarrinho(const QString& id);
    void mostrarCarrinho();
    void atualizarQuantidadeCarrinho(const QString& id, int delta);
    void removerDoCarrinho(const QString& id);

    // Login/Admin
    void handleLogin();
    void abrirLogin();
        void criarConta(); // New slot to open create-account dialog
    void logoutUser();
    void solicitarAdmin();
    void tentarLoginAdmin(const QString& senha);
    void logoutAdmin();

private:
    void atualizarCarrinhoIcon();
    void atualizarCarrinhoPagina();

    QMap<QString, int> carrinho; // id -> quantidade
    ClickableLabel* carrinhoIconLabel;
    QPushButton* loginButton;
    QStackedWidget* paginas;
    QWidget* lojaPage;
    QWidget* carrinhoPage;
    QWidget* loginPage = nullptr;
    QLineEdit* loginUsernameEdit = nullptr;
    QLineEdit* loginPasswordEdit = nullptr;
    QWidget* sobrePage;
    QWidget* inicioPage;
    QWidget* contatoPage;
    QLabel* userNameLabel = nullptr;
    QTabWidget* adminTabWidget = nullptr;
    QListWidget* adminOrdersList = nullptr;
    void mostrarEncomendas();
    void setupUserMenu();
    void setupAdminPage();
    void setupAdminOrdersTab();
    void atualizarListaEncomendas();

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
    
        // helpers for user persistence
    bool validarCredenciais(const QString& username, const QString& password);
    bool salvarUsuario(const QString& username, const QString& password,
              const QString& fullName, const QString& email,
              const QString& phone, const QString& nif,
              QString& outError);

    // User session
    QString loggedInUser;

    // Admin page and state
    QWidget* adminPage;
    bool isAdmin = false;

    QTimer* reservationTimer = nullptr;
    QVBoxLayout* mainLayout = nullptr; // Layout principal da janela
    QList<QPushButton*> menuButtons; // Lista de botões do menu principal

    // stock / reservations
    void checkReservations();
    void showLowStockPanel();
    void notifyAdminLowStock(const ProdutoFull& p);

    // Order management
    void finalizarCompra();
    void mostrarDetalhesEncomenda(const QString& orderId);
    bool salvarEncomenda(const Order& order, QString& outError);
    QString gerarOrderId();
    QVector<Order> carregarEncomendas();
    void limparCarrinho();
};

#endif // MAINWINDOW_H