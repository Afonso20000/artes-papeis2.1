#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QDateTime>
#include <QLocale>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    createMenus();

    // Produtos adicionados apenas por código (interface é apenas para visualização/clientes)
    addProductFromCode("Vela da Vida", 7.50);
    addProductFromCode("Concha de Baptismo", 12.00);
    addProductFromCode("Convites de Casamento", 1.20);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Lista de produtos (somente leitura para o cliente)
    productList = new QListWidget(this);
    productList->setSelectionMode(QAbstractItemView::SingleSelection);
    productList->setContextMenuPolicy(Qt::NoContextMenu);
    mainLayout->addWidget(productList);
    
    // Label de total
    totalLabel = new QLabel(this);
    mainLayout->addWidget(totalLabel);

    // Inicializar total
    total = 0.0;
    QLocale l = QLocale::system();
    totalLabel->setText(QString("Total: %1").arg(l.toString(total, 'f', 2) + "€"));
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&Arquivo");
    QAction *exitAction = fileMenu->addAction("Sair");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu *helpMenu = menuBar()->addMenu("&Ajuda");
    QAction *aboutAction = helpMenu->addAction("Sobre");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "Sobre", "Loja de Artesanato — Interface de cliente\nVersão 1.0");
    });
}

void MainWindow::addProductFromCode(const QString &name, double price) {
    // Apresenta o produto na lista e atualiza o total.
    // Produtos só devem ser adicionados por chamadas a este método.
    QString text = QString("%1 — %2€").arg(name).arg(QString::number(price, 'f', 2));
    QListWidgetItem *item = new QListWidgetItem(text, productList);

    // Guardar o preço nos dados do item para usos futuros
    item->setData(Qt::UserRole, price);

    // Tornar o item não editável pelo usuário
    item->setFlags(item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);

    // Atualizar total (opcional: se quiser apenas mostrar lista retire isto)
    total += price;
    QLocale l = QLocale::system();
    totalLabel->setText(QString("Total: %1").arg(l.toString(total, 'f', 2) + "€"));
}