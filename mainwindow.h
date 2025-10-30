#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>

class QStackedWidget;
class QLabel;

struct Produto {
    QString nome;
    double preco;
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
};

#endif // MAINWINDOW_H