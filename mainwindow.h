#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QColor>

class QStackedWidget;
class QLabel;
class QWidget;
class QPushButton;
class QHBoxLayout;
class QScrollArea;

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
    QString imagePath;    // relative path to stored image (optional)
    QString categoria;
};

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
    QVector<ProdutoFull> produtosDisponiveis;
    QWidget* productsWidget = nullptr;
    QHBoxLayout* productsFlow = nullptr;
    QScrollArea* productsScroll = nullptr;
    QPushButton* editProductsButton = nullptr; // visível apenas para admin

    // Gerir produtos (apenas admin)
    void refreshLojaProducts();
    void showProductManager();
    void updateAdminUI();
    void saveProductsToJson();
    void loadProductsFromJson();
    void logoutAdmin();

    // Admin page and state
    QWidget* adminPage;
    bool isAdmin = false;
    QPushButton* adminButton = nullptr;
};

#endif // MAINWINDOW_H