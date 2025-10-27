#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    createMenus();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Product list
    productList = new QListWidget(this);
    mainLayout->addWidget(productList);
    
    // Buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addButton = new QPushButton("Adicionar Produto", this);
    removeButton = new QPushButton("Remover Produto", this);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    mainLayout->addLayout(buttonLayout);
    
    // Total label
    totalLabel = new QLabel("Total: 0.00€", this);
    mainLayout->addWidget(totalLabel);
    
    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addProduct);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeProduct);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&Arquivo");
    QAction *exitAction = fileMenu->addAction("Sair");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu *helpMenu = menuBar()->addMenu("&Ajuda");
    QAction *aboutAction = helpMenu->addAction("Sobre");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "Sobre", "Loja de Artesanato v1.0");
    });
}

void MainWindow::addProduct() {
    // TODO: Implement add product dialog
    productList->addItem("Novo Produto");
}

void MainWindow::removeProduct() {
    // Remove selected product
    QListWidgetItem *currentItem = productList->currentItem();
    if (currentItem) {
        delete currentItem;
    }
}