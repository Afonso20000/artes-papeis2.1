#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QLineEdit>
#include <QMap>
#include "produto.h"
#include "productmanager.h"
#include "clickablelabel.h"

class QStackedWidget;
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
    void criarConta();
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
        bool salvarUsuario(const QString& username, const QString& password, QString& outError);

    // User session
    QString loggedInUser;

    // Admin page and state
    QWidget* adminPage;
    bool isAdmin = false;
    QPushButton* adminButton = nullptr;
};

#endif // MAINWINDOW_H