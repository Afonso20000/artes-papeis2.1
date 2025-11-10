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
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QTextEdit>

// Qt Core
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QVector>

// Local includes
#include "produto.h"
#include "productmanager.h"
#include "clickablelabel.h"

// Estrutura para mensagens do chat
struct ChatMessage {
    QString userId;
    QString userName;
    QString message;
    QDateTime timestamp;
};

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
    QString status; // "pending", "accepted", "rejected"
    QVector<ChatMessage> chat; // Mensagens do chat
    QDateTime lastUpdated;
};

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

// Forward declaration
class MainWindow;

// Event filter helper para widgets clicáveis de encomenda
class ClickableOrderWidget : public QObject
{
    Q_OBJECT
public:
    ClickableOrderWidget(QObject* parent, const QString& orderId) 
        : QObject(parent), m_orderId(orderId) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QString m_orderId;
};

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
    void filtrarProdutos(const QString& searchText);
    void filtrarPorCategoria(const QString& categoria);
    void mostrarPreviewCarrinho();
    void exportarRelatoriosCSV();
    void mostrarDetalhesEncomenda(const QString& orderId); // Público para acesso do ClickableOrderWidget

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
    void setupDashboardTab();
    void setupInventoryTab();
    void setupReportsTab();
    void atualizarListaEncomendas();
    void filtrarEncomendas(const QString& searchText);
    void mostrarPreviewPesquisa(const QString& searchText);
    void esconderPreviewPesquisa();
    void atualizarDashboard();
    void atualizarEstoque();
    void atualizarRelatorios();
    
    // Métodos auxiliares para o dashboard
    double calcularVendasTotais();
    QMap<QString, int> obterProdutosMaisVendidos(int limite = 5);
    QMap<QString, int> obterClientesMaisAtivos(int limite = 5);
    QMap<QDate, double> obterVendasPorPeriodo(int dias = 30);
    QString obterTextoEstadoEncomenda(const QString& status);
    QColor obterCorEstadoEncomenda(const QString& status);

    // Loja: produtos geridos por código, mutáveis via admin
    ProductManager* productManager;
    QWidget* productsWidget = nullptr;
    QGridLayout* productsGrid = nullptr;
    QScrollArea* productsScroll = nullptr;
    QPushButton* editProductsButton = nullptr; // visível apenas para admin
    QLineEdit* searchBar = nullptr; // Barra de pesquisa
    QWidget* searchPreviewWidget = nullptr; // Widget para preview de produtos na pesquisa
    QString currentCategory = "Todos"; // Categoria atualmente selecionada
    QString currentOrderStatusFilter = "Todos"; // Filtro de status de encomendas
    QWidget* categoryButtonsWidget = nullptr; // Widget com botões de categoria

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
    QPushButton* adminMenuButton = nullptr; // Botão Admin no menu (para badge)
    void atualizarBadgeAdmin(); // Atualizar contador de notificações

    // stock / reservations
    void checkReservations();
    void showLowStockPanel();
    void notifyAdminLowStock(const ProdutoFull& p);

    // Order management
    void finalizarCompra();
    bool salvarEncomenda(const Order& order, QString& outError);
    bool atualizarEncomendas(const QVector<Order>& orders, QString& outError);
    QString gerarOrderId();
    QVector<Order> carregarEncomendas();
    void limparCarrinho();
};

#endif // MAINWINDOW_H