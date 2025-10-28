#include "mainwindow.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>

QWidget* criarCard(MainWindow* win, const QString& nome, const QString& preco, const QColor& cor) {
    QWidget* card = new QWidget();
    card->setFixedSize(210, 320);
    QVBoxLayout* layout = new QVBoxLayout(card);

    QLabel* img = new QLabel(card);
    img->setFixedSize(175, 110);
    img->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(cor.name()));
    img->setAlignment(Qt::AlignCenter);
    img->setText("Imagem");

    QLabel* title = new QLabel(nome, card);
    title->setStyleSheet("font-size: 17px; font-weight: bold; color: #0E141C; margin-top: 10px;");
    title->setAlignment(Qt::AlignHCenter);

    QLabel* priceLabel = new QLabel(preco + " €", card);
    priceLabel->setStyleSheet("font-size: 16px; color: #607EA2;");
    priceLabel->setAlignment(Qt::AlignHCenter);

    QPushButton* btn = new QPushButton("Comprar", card);
    btn->setStyleSheet("background-color: #BDB3A3; color: #0E141C; border-radius: 8px; padding: 6px 15px; font-weight: bold;");
    QObject::connect(btn, &QPushButton::clicked, win, [win, nome, preco]() {
        win->adicionarAoCarrinho(nome, preco.toDouble());
    });

    layout->addWidget(img, 0, Qt::AlignHCenter);
    layout->addWidget(title);
    layout->addWidget(priceLabel);
    layout->addStretch(1);
    layout->addWidget(btn, 0, Qt::AlignHCenter);

    card->setLayout(layout);
    card->setStyleSheet("background: #F4F4F4; border-radius: 14px; border: 1px solid #8197AC; padding: 11px;");
    return card;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), carrinhoIconLabel(nullptr)
{
    QColor cBlack("#0E141C"), pBlue("#314B6E"), rackley("#607EA2"), weldon("#8197AC"), sPink("#BDB3A3");

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Cabeçalho e navegação igual exemplo anterior
    QLabel* header = new QLabel("Frete grátis para todo o mundo em pedidos acima de 50€");
    header->setStyleSheet("background-color: #0E141C; color: #BDB3A3; padding: 7px; font-size: 14px; letter-spacing: 1.1px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QHBoxLayout* navLayout = new QHBoxLayout();
    QLineEdit* searchBar = new QLineEdit();
    searchBar->setPlaceholderText("Buscar");
    searchBar->setFixedWidth(210);

    QLabel* logo = new QLabel("<b>Loja Artesanatos</b>");
    logo->setStyleSheet("font-size: 24px; color: #314B6E;");

    carrinhoIconLabel = new QLabel("🛒 Carrinho (0)");
    carrinhoIconLabel->setStyleSheet("color: #314B6E; font-size: 15px; text-decoration: underline;");
    carrinhoIconLabel->setCursor(Qt::PointingHandCursor);
    connect(carrinhoIconLabel, &QLabel::linkActivated, this, &MainWindow::mostrarCarrinho);

    navLayout->addWidget(searchBar, 0);
    navLayout->addStretch(1);
    navLayout->addWidget(logo, 0, Qt::AlignCenter);
    navLayout->addStretch(1);
    navLayout->addWidget(carrinhoIconLabel, 0);

    mainLayout->addLayout(navLayout);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // StackedWidget para as páginas principais
    paginas = new QStackedWidget(this);

    // Página: Loja (produtos)
    lojaPage = new QWidget;
    QVBoxLayout* lojaLayout = new QVBoxLayout(lojaPage);
    QLabel* sectionTitle = new QLabel("Comprar coleções");
    sectionTitle->setStyleSheet("font-size: 23px; margin: 23px 0 16px 0; font-weight: bold; color: #0E141C;");
    sectionTitle->setAlignment(Qt::AlignCenter);
    lojaLayout->addWidget(sectionTitle);

    QHBoxLayout* productsLayout = new QHBoxLayout();
    QWidget* productsWidget = new QWidget;
    QHBoxLayout* flow = new QHBoxLayout(productsWidget);
    flow->addSpacing(25);

    QVector<QPair<QString, QPair<QString, QColor>>> produtos = {
        { "Vela da Vida", { "7.50", weldon } },
        { "Concha de Batismo", { "12.00", rackley } },
        { "Convites", { "1.20", pBlue } },
        { "Caixa de Madeira", { "9.50", sPink } }
    };
    for (const auto& prod : produtos){
        flow->addWidget(criarCard(this, prod.first, prod.second.first, prod.second.second));
        flow->addSpacing(12);
    }
    productsWidget->setLayout(flow);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidget(productsWidget);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->setFixedHeight(350);

    lojaLayout->addWidget(scroll);
    lojaPage->setLayout(lojaLayout);

    // Página do carrinho
    carrinhoPage = new QWidget;
    QVBoxLayout* carrinhoLayout = new QVBoxLayout(carrinhoPage);
    carrinhoPage->setLayout(carrinhoLayout);

    // Página do blog (apresentação simples)
    blogPage = new QWidget;
    QVBoxLayout* blogLayout = new QVBoxLayout(blogPage);
    QLabel* blogTitle = new QLabel("Blog Artesanal");
    blogTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #0E141C; margin: 14px;");
    blogLayout->addWidget(blogTitle);
    QTextEdit* blogContent = new QTextEdit();
    blogContent->setText("Bem-vindo ao nosso blog!\n\nAqui partilhamos novidades e dicas sobre artesanato.");
    blogContent->setReadOnly(true);
    blogContent->setStyleSheet("background-color: #F4F4F4; color: #314B6E; font-size: 15px;");
    blogLayout->addWidget(blogContent);

    // Página sobre
    sobrePage = new QWidget;
    QVBoxLayout* sobreLayout = new QVBoxLayout(sobrePage);
    QLabel* sobreTitle = new QLabel("Sobre a Loja");
    sobreTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #0E141C; margin: 14px;");
    sobreLayout->addWidget(sobreTitle);
    QLabel* sobreContent = new QLabel("Loja dedicada ao artesanato local!\nFundada em 2025.");
    sobreContent->setStyleSheet("color: #314B6E; font-size: 16px;");
    sobreContent->setAlignment(Qt::AlignCenter);
    sobreLayout->addWidget(sobreContent);

    // Página início
    inicioPage = new QWidget;
    QVBoxLayout* inicioLayout = new QVBoxLayout(inicioPage);
    QLabel* inicioTitle = new QLabel("Bem-vindo à Loja de Artesanatos!");
    inicioTitle->setStyleSheet("font-size: 24px; color: #314B6E; margin: 20px;");
    inicioLayout->addWidget(inicioTitle);
    inicioLayout->addWidget(new QLabel("Aproveite as nossas coleções exclusivas.", inicioPage));

    // Página contato
    contatoPage = new QWidget;
    QVBoxLayout* contatoLayout = new QVBoxLayout(contatoPage);
    QLabel* contatoTitle = new QLabel("Contacte-nos");
    contatoTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #0E141C;");
    contatoLayout->addWidget(contatoTitle);
    QLabel* contatoContent = new QLabel("Email: a39694@aepombal.edu.pt\nTelemóvel: +351 928 052 266");
    contatoContent->setStyleSheet("color: #314B6E; font-size: 16px;");
    contatoLayout->addWidget(contatoContent);

    paginas->addWidget(inicioPage);
    paginas->addWidget(lojaPage);
    paginas->addWidget(carrinhoPage);
    paginas->addWidget(blogPage);
    paginas->addWidget(sobrePage);
    paginas->addWidget(contatoPage);

    mainLayout->addWidget(paginas);

    mainLayout->addStretch(1);
    QLabel* footer = new QLabel("© 2025 Loja Artesanatos");
    footer->setStyleSheet("color: #8197AC; font-size: 13px; margin-top: 27px;");
    footer->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footer);

    central->setLayout(mainLayout);
    setCentralWidget(central);
    setStyleSheet("background-color: #F4F4F4;");
    resize(1200, 790);

    // Menu funcional
    QHBoxLayout* menuLayout = new QHBoxLayout();
    QStringList labels = {"Início", "Loja", "Carrinho", "Blog", "Sobre", "Contato"};
    for (int i = 0; i < labels.size(); ++i) {
        QPushButton* btn = new QPushButton(labels[i]);
        btn->setStyleSheet("background: none; border: none; color: #607EA2; font-size: 17px; font-weight: 600; padding: 0 18px;");
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            paginas->setCurrentIndex(i);
            if (i == 2) atualizarCarrinhoPagina();
        });
        menuLayout->addWidget(btn);
    }
    mainLayout->insertLayout(3, menuLayout);  // Logo abaixo do separador

    // Página inicial por padrão
    paginas->setCurrentIndex(0);
}

void MainWindow::abrirBlog()    { paginas->setCurrentWidget(blogPage);        }
void MainWindow::abrirSobre()   { paginas->setCurrentWidget(sobrePage);       }
void MainWindow::abrirLoja()    { paginas->setCurrentWidget(lojaPage);        }
void MainWindow::abrirInicio()  { paginas->setCurrentWidget(inicioPage);      }
void MainWindow::abrirContato() { paginas->setCurrentWidget(contatoPage);     }

void MainWindow::adicionarAoCarrinho(const QString& nome, double preco) {
    Produto p{nome, preco};
    carrinho.append(p);
    atualizarCarrinhoIcon();
}

void MainWindow::atualizarCarrinhoIcon() {
    carrinhoIconLabel->setText(QString("🛒 Carrinho (%1)").arg(carrinho.size()));
}

void MainWindow::mostrarCarrinho() {
    paginas->setCurrentWidget(carrinhoPage);
    atualizarCarrinhoPagina();
}

void MainWindow::atualizarCarrinhoPagina() {
    auto* layout = qobject_cast<QVBoxLayout*>(carrinhoPage->layout());
    // Remove widgets exceto o primeiro (título)
    while (layout->count() > 1) {
        QLayoutItem* item = layout->takeAt(1);
        if (auto w = item->widget()) w->deleteLater();
        delete item;
    }
    if (carrinho.isEmpty()) {
        layout->addWidget(new QLabel("O seu carrinho está vazio."));
    } else {
        double total = 0;
        for (const auto& p: carrinho) {
            QLabel* prodLbl = new QLabel(QString("• %1: %.2f€").arg(p.nome).arg(p.preco));
            prodLbl->setStyleSheet("font-size: 15px; color: #314B6E;");
            layout->addWidget(prodLbl);
            total += p.preco;
        }
        QLabel* totalLbl = new QLabel(QString("<b>Total: %.2f€</b>").arg(total));
        totalLbl->setStyleSheet("font-size: 16px; color: #0E141C; padding-top:10px;");
        layout->addWidget(totalLbl);
    }
}

MainWindow::~MainWindow(){}