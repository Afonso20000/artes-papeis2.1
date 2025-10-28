#include "mainwindow.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QSpacerItem>
#include <QStyle>
#include <QFont>

QWidget* createProductCard(const QString& name, const QString& price, const QColor& color, QWidget* parent = nullptr) {
    QWidget* card = new QWidget(parent);
    card->setFixedSize(210, 290);
    QVBoxLayout* layout = new QVBoxLayout(card);

    QLabel* img = new QLabel(card);
    img->setFixedSize(175, 110);
    img->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(color.name()));
    img->setAlignment(Qt::AlignCenter);
    img->setText("Imagem");

    QLabel* title = new QLabel(name, card);
    title->setStyleSheet("font-size: 17px; font-weight: bold; color: #0E141C; margin-top: 8px;");
    title->setAlignment(Qt::AlignHCenter);

    QLabel* priceLabel = new QLabel(price + " €", card);
    priceLabel->setStyleSheet("font-size: 16px; color: #607EA2;");

    QPushButton* btn = new QPushButton("Ver tudo", card);
    btn->setStyleSheet("background-color: #BDB3A3; color: #0E141C; border-radius: 8px; padding: 6px 15px; font-weight: bold;");

    layout->addWidget(img, 0, Qt::AlignHCenter);
    layout->addWidget(title);
    layout->addWidget(priceLabel, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    layout->addWidget(btn, 0, Qt::AlignHCenter);

    card->setLayout(layout);
    card->setStyleSheet("background: #F4F4F4; border-radius: 14px; border: 1px solid #8197AC; padding: 11px;");
    return card;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // Paleta das cores principais
    QColor cBlack("#0E141C"), pBlue("#314B6E"), rackley("#607EA2"), weldon("#8197AC"), sPink("#BDB3A3");

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Cabeçalho fino
    QLabel* header = new QLabel("Frete grátis para todo o mundo em pedidos acima de 50€");
    header->setStyleSheet("background-color: #0E141C; color: #BDB3A3; padding: 7px; font-size: 14px; letter-spacing: 1.1px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    // Barra navegação horizontal
    QHBoxLayout* navLayout = new QHBoxLayout();
    QLineEdit* searchBar = new QLineEdit();
    searchBar->setPlaceholderText("Buscar");
    searchBar->setFixedWidth(210);

    QLabel* logo = new QLabel("<b>Loja Artesanatos</b>");
    logo->setStyleSheet("font-size: 24px; color: #314B6E;");
    QLabel* cart = new QLabel("🛒 Carrinho (0)");
    cart->setStyleSheet("color: #314B6E; font-size: 15px;");

    navLayout->addWidget(searchBar, 0);
    navLayout->addStretch(1);
    navLayout->addWidget(logo, 0, Qt::AlignCenter);
    navLayout->addStretch(1);
    navLayout->addWidget(cart, 0);

    mainLayout->addLayout(navLayout);

    // Menu tipo site
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    QHBoxLayout* menuLayout = new QHBoxLayout();
    QStringList pages = { "Início", "Loja", "Sobre", "Blog", "Contato" };
    for (const QString& page : pages) {
        QPushButton* btn = new QPushButton(page);
        btn->setStyleSheet("background: none; border: none; color: #607EA2; font-size: 17px; font-weight: 600; padding: 0 20px;");
        menuLayout->addWidget(btn);
    }
    mainLayout->addLayout(menuLayout);

    // Título da secção
    QLabel* sectionTitle = new QLabel("Comprar coleções");
    sectionTitle->setStyleSheet("font-size: 23px; margin: 23px 0 16px 0; font-weight: bold; color: #0E141C;");
    sectionTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(sectionTitle);

    // Cards dos produtos
    QHBoxLayout* productsLayout = new QHBoxLayout();
    productsLayout->addSpacing(25);
    productsLayout->addWidget(createProductCard("Vela da Vida", "7.50", weldon));
    productsLayout->addSpacing(10);
    productsLayout->addWidget(createProductCard("Concha de Batismo", "12.00", rackley));
    productsLayout->addSpacing(10);
    productsLayout->addWidget(createProductCard("Convites", "1.20", pBlue));
    productsLayout->addSpacing(10);
    productsLayout->addWidget(createProductCard("Caixa de Madeira", "9.50", sPink));
    productsLayout->addSpacing(25);
    mainLayout->addLayout(productsLayout);

    // Rodapé
    mainLayout->addStretch(1);
    QLabel* footer = new QLabel("© 2025 Loja Artesanatos");
    footer->setStyleSheet("color: #8197AC; font-size: 13px; margin-top: 27px;");
    footer->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footer);

    central->setLayout(mainLayout);
    setCentralWidget(central);

    // Fundo principal
    setStyleSheet("background-color: #F4F4F4;");
    resize(1200, 700);
}

MainWindow::~MainWindow() {}