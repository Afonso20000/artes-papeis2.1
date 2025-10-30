#include "mainwindow.h"
#include "productcard.h"
#include "productmanager.h"
#include "productformdialog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QDialog>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QUuid>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), carrinhoIconLabel(nullptr), adminPage(nullptr), productManager(new ProductManager(this))
{
    QColor cBlack("#0E141C"), pBlue("#314B6E"), rackley("#607EA2"), weldon("#8197AC"), sPink("#BDB3A3");

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

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

    paginas = new QStackedWidget(this);

    // --------- Página LOJA -----------
    lojaPage = new QWidget;
    QVBoxLayout* lojaLayout = new QVBoxLayout(lojaPage);
    QLabel* sectionTitle = new QLabel("Comprar coleções");
    sectionTitle->setStyleSheet("font-size: 23px; margin: 23px 0 16px 0; font-weight: bold; color: #0E141C;");
    sectionTitle->setAlignment(Qt::AlignCenter);

    // Botão de editar produtos (aparece apenas para admin)
    editProductsButton = new QPushButton("Editar Produtos");
    editProductsButton->setVisible(false);
    editProductsButton->setStyleSheet("background-color: #C0392B; color: white; padding: 6px 12px; border-radius: 6px;");
    connect(editProductsButton, &QPushButton::clicked, this, &MainWindow::showProductManager);

    // header horizontal para título + edit button
    QHBoxLayout* titleRow = new QHBoxLayout();
    titleRow->addWidget(sectionTitle, 0, Qt::AlignLeft);
    titleRow->addStretch(1);
    titleRow->addWidget(editProductsButton, 0, Qt::AlignRight);
    lojaLayout->addLayout(titleRow);

    // Configurar o ProductManager para gerir nossos produtos
    productManager = new ProductManager(this);
    connect(productManager, &ProductManager::productsChanged, this, &MainWindow::refreshLojaProducts);

    // Se não houver produtos, adicionar alguns padrão
    auto produtos = productManager->getAllProducts();
    if (produtos.isEmpty()) {
        productManager->addProduct({ "Vela da Vida", 7.50, weldon, "V001", 10, QString(), QString("Geral") });
        productManager->addProduct({ "Concha de Batismo", 12.00, rackley, "C002", 5, QString(), QString("Geral") });
        productManager->addProduct({ "Convites", 1.20, pBlue, "CV03", 200, QString(), QString("Geral") });
        productManager->addProduct({ "Caixa de Madeira", 9.50, sPink, "CX04", 8, QString(), QString("Geral") });
    }

    // Widget/grid usado para mostrar os cards (será atualizado por refreshLojaProducts)
    productsWidget = new QWidget;
    productsGrid = new QGridLayout(productsWidget);
    productsGrid->setContentsMargins(20, 10, 20, 10);
    productsGrid->setHorizontalSpacing(18);
    productsGrid->setVerticalSpacing(18);

    productsScroll = new QScrollArea();
    productsScroll->setWidget(productsWidget);
    productsScroll->setWidgetResizable(true);
    // esconder scroll horizontal e permitir scroll vertical quando necessário
    productsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    productsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    productsScroll->setFixedHeight(520);

    lojaLayout->addWidget(productsScroll);
    lojaPage->setLayout(lojaLayout);

    // Preencher a lista de produtos inicial
    refreshLojaProducts();

    // --------- Página CARRINHO ---------
    carrinhoPage = new QWidget;
    QVBoxLayout* carrinhoLayout = new QVBoxLayout(carrinhoPage);
    carrinhoPage->setLayout(carrinhoLayout);

    // --------- Página BLOG -------------
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

    // --------- Página SOBRE -------------
    sobrePage = new QWidget;
    QVBoxLayout* sobreLayout = new QVBoxLayout(sobrePage);
    QLabel* sobreTitle = new QLabel("Sobre a Loja");
    sobreTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #0E141C; margin: 14px;");
    sobreLayout->addWidget(sobreTitle);
    QLabel* sobreContent = new QLabel("Loja dedicada ao artesanato local!\nFundada em 2025.");
    sobreContent->setStyleSheet("color: #314B6E; font-size: 16px;");
    sobreContent->setAlignment(Qt::AlignCenter);
    sobreLayout->addWidget(sobreContent);

    // --------- Página INÍCIO ------------
    inicioPage = new QWidget;
    QVBoxLayout* inicioLayout = new QVBoxLayout(inicioPage);
    QLabel* inicioTitle = new QLabel("Bem-vindo à Loja de Artesanatos!");
    inicioTitle->setStyleSheet("font-size: 24px; color: #314B6E; margin: 20px;");
    inicioLayout->addWidget(inicioTitle);
    QLabel* destaqueLbl = new QLabel("Aproveite as nossas coleções exclusivas.", inicioPage);
    destaqueLbl->setStyleSheet("color: #314B6E; font-size: 16px;");
    inicioLayout->addWidget(destaqueLbl);

    // --------- Página CONTATO -----------
    contatoPage = new QWidget;
    QVBoxLayout* contatoLayout = new QVBoxLayout(contatoPage);
    QLabel* contatoTitle = new QLabel("Contacte-nos");
    contatoTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #0E141C;");
    contatoLayout->addWidget(contatoTitle);
    QLabel* contatoContent = new QLabel("Email: artes@loja.com\nTelemóvel: 999-999-999");
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

    QHBoxLayout* menuLayout = new QHBoxLayout();
    QStringList labels = {"Início", "Loja", "Carrinho", "Blog", "Sobre", "Contato"};
    for (int i = 0; i < labels.size(); ++i) {
        QPushButton* btn = new QPushButton(labels[i]);
        btn->setStyleSheet(R"(
            QPushButton {
                background: none;
                border: none;
                color: #314B6E;
                font-size: 17px;
                font-weight: bold;
                padding: 8px 23px;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #BDB3A3;
                color: #0E141C;
                cursor: pointer;
            }
            QPushButton:pressed {
                background-color: #607EA2;
                color: #0E141C;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            paginas->setCurrentIndex(i);
            if (i == 2) atualizarCarrinhoPagina();
        });
        menuLayout->addWidget(btn);
    }
    // Botão de Admin (pede password)
    QPushButton* adminBtn = new QPushButton("Admin");
    adminBtn->setStyleSheet(R"(
        QPushButton {
            background: none;
            border: 1px solid #E0E0E0;
            color: #C0392B;
            font-size: 14px;
            padding: 6px 12px;
            border-radius: 6px;
        }
        QPushButton:hover { background-color: #F2D7D5; }
    )");
    // store member pointer so updateAdminUI can toggle behavior
    adminButton = adminBtn;
    connect(adminBtn, &QPushButton::clicked, this, &MainWindow::solicitarAdmin);
    menuLayout->addWidget(adminBtn);
    mainLayout->insertLayout(3, menuLayout);

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
    while (layout->count() > 1) {
        QLayoutItem* item = layout->takeAt(1);
        if (auto w = item->widget()) w->deleteLater();
        delete item;
    }
    if (carrinho.isEmpty()) {
        QLabel* vazioLbl = new QLabel("O seu carrinho está vazio.");
        vazioLbl->setStyleSheet("color: #0E141C; font-size: 18px; font-weight: bold;");
        layout->addWidget(vazioLbl);
    } else {
        double total = 0;
        for (const auto& p: carrinho) {
            QString precoStr = QString::number(p.preco, 'f', 2);
            QLabel* prodLbl = new QLabel(QString("• %1: %2€").arg(p.nome).arg(precoStr));
            prodLbl->setStyleSheet("font-size: 15px; color: #314B6E;");
            layout->addWidget(prodLbl);
            total += p.preco;
        }
        QLabel* totalLbl = new QLabel(QString("<b>Total: %1€</b>").arg(QString::number(total, 'f', 2)));
        totalLbl->setStyleSheet("font-size: 16px; color: #0E141C; padding-top:10px;");
        layout->addWidget(totalLbl);
    }
}

void MainWindow::solicitarAdmin()
{
    // Usamos uma instância de QInputDialog para podermos aplicar estilo localmente
    QInputDialog dlg(this);
    dlg.setWindowTitle("Acesso Admin");
    dlg.setLabelText("Password:");
    dlg.setTextEchoMode(QLineEdit::Password);

    // Definir cores para melhorar contraste: cor do texto e do campo de input.
    // Ajuste os valores (#0E141C é um tom escuro; pode trocar para #FFFFFF para branco).
    dlg.setStyleSheet(
        "QInputDialog QLabel { color: #0E141C; font-weight: bold; }"
        "QInputDialog QLineEdit { color: #0E141C; background: #FFFFFF; selection-background-color: #607EA2; }"
    );

    if (dlg.exec() != QDialog::Accepted) return; // utilizador cancelou
    QString senha = dlg.textValue();
    tentarLoginAdmin(senha);
}

void MainWindow::tentarLoginAdmin(const QString& senha)
{
    // Autenticação por ficheiro (data/admin.json com salt+hash)
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists()) dataDir.mkpath(".");
    QString adminPath = dataDir.filePath("admin.json");

    // Se ficheiro não existir, cria com password por defeito 'admin123'
    if (!QFile::exists(adminPath)) {
        QString defaultPass = "admin123";
        QString salt = QUuid::createUuid().toString();
        QByteArray h = QCryptographicHash::hash((salt + defaultPass).toUtf8(), QCryptographicHash::Sha256);
        QJsonObject obj;
        obj["salt"] = salt;
        obj["hash"] = QString(h.toHex());
        QFile f(adminPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(obj).toJson());
            f.close();
        }
    }

    // Ler credenciais
    QFile f(adminPath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erro", "Não foi possível ler credenciais de admin.");
        return;
    }
    QByteArray content = f.readAll();
    f.close();
    QJsonDocument jd = QJsonDocument::fromJson(content);
    if (!jd.isObject()) {
        QMessageBox::warning(this, "Erro", "Ficheiro de credenciais inválido.");
        return;
    }
    QJsonObject jo = jd.object();
    QString salt = jo.value("salt").toString();
    QString expected = jo.value("hash").toString();

    QByteArray hinput = QCryptographicHash::hash((salt + senha).toUtf8(), QCryptographicHash::Sha256);
    QString hhex = QString(hinput.toHex());
    if (hhex == expected) {
        isAdmin = true;
        updateAdminUI();
        QMessageBox::information(this, "Acesso Admin", "Password correta. Acesso concedido.");

        // Criar a página de admin, se ainda não existir
        if (!adminPage) {
            adminPage = new QWidget;
            QVBoxLayout* layout = new QVBoxLayout(adminPage);
            QLabel* lbl = new QLabel("Área Administrativa — Acesso concedido", adminPage);
            lbl->setStyleSheet("font-size: 20px; font-weight: bold; color: #C0392B;");
            layout->addWidget(lbl, 0, Qt::AlignCenter);

            // Exemplo de controlo admin (apenas demonstrativo)
            QPushButton* demoBtn = new QPushButton("Adicionar produto demo ao carrinho", adminPage);
            connect(demoBtn, &QPushButton::clicked, this, [this]() {
                adicionarAoCarrinho("Produto Admin (demo)", 99.99);
                QMessageBox::information(this, "Demo", "Produto demo adicionado ao carrinho.");
            });
            layout->addWidget(demoBtn, 0, Qt::AlignCenter);
            layout->addStretch(1);
            adminPage->setLayout(layout);
        }

        // Adiciona a página ao stacked widget (se ainda não estiver) e mostra
        paginas->addWidget(adminPage);
        paginas->setCurrentWidget(adminPage);
    } else {
        QMessageBox::warning(this, "Acesso Negado", "Password incorreta.");
    }
}

void MainWindow::refreshLojaProducts()
{
    if (!productsGrid || !productsWidget) return;
    // Limpar conteúdos existentes do layout (remover widgets anteriores)
    while (productsGrid->count() > 0) {
        QLayoutItem* it = productsGrid->takeAt(0);
        if (!it) break;
        if (QWidget* w = it->widget()) { w->deleteLater(); }
        delete it;
    }

    QVector<ProdutoFull> produtos = productManager->getAllProducts();
    const int cols = 4; // itens por linha
    int idx = 0;
    for (const auto &pf : produtos) {
        ProductCard* card = new ProductCard(pf, this);
        connect(card, &ProductCard::compraProduto, this, [this, pf](const QString&) {
            adicionarAoCarrinho(pf.nome, pf.preco);
        });
        int row = idx / cols;
        int col = idx % cols;
        productsGrid->addWidget(card, row, col, Qt::AlignTop);
        ++idx;
    }
}

void MainWindow::showProductManager()
{
    if (!isAdmin) {
        QMessageBox::warning(this, "Acesso Negado", "É necessário ser admin para gerir produtos.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Gerir Produtos");
    dlg.resize(480, 400);

    QVBoxLayout* main = new QVBoxLayout(&dlg);
    QListWidget* list = new QListWidget(&dlg);
    
    auto produtos = productManager->getAllProducts();
    for (const auto &pf : produtos) {
        list->addItem(QString("%1 [id:%2] x%3 — %4€")
            .arg(pf.nome)
            .arg(pf.id)
            .arg(pf.quantidade)
            .arg(QString::number(pf.preco, 'f', 2)));
    }
    main->addWidget(list);

    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Adicionar", &dlg);
    QPushButton* editBtn = new QPushButton("Editar", &dlg);
    QPushButton* removeBtn = new QPushButton("Remover", &dlg);
    QPushButton* closeBtn = new QPushButton("Fechar", &dlg);

    btnRow->addWidget(addBtn);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(closeBtn);
    main->addLayout(btnRow);

    // Tema escuro para o diálogo
    dlg.setStyleSheet(R"(
        QDialog { background-color: #2b2b2b; color: #eaeaea; }
        QLabel { color: #eaeaea; }
        QListWidget { background-color: #3a3a3a; color: #eaeaea; }
        QLineEdit, QSpinBox, QDoubleSpinBox { background-color: #4a4a4a; color: #eaeaea; }
        QPushButton { background-color: #5a5a5a; color: #ffffff; padding: 6px 10px; border-radius: 4px; }
        QPushButton:hover { background-color: #707070; }
    )");

    connect(addBtn, &QPushButton::clicked, this, [&]() {
        ProductFormDialog formDlg(this);
        if (formDlg.exec() == QDialog::Accepted) {
            ProdutoFull produto = formDlg.getProduct();
            productManager->addProduct(produto);
            list->addItem(QString("%1 [id:%2] x%3 — %4€")
                .arg(produto.nome)
                .arg(produto.id)
                .arg(produto.quantidade)
                .arg(QString::number(produto.preco, 'f', 2)));
        }
    });

    connect(removeBtn, &QPushButton::clicked, this, [&]() {
        int idx = list->currentRow();
        if (idx >= 0 && idx < produtos.size()) {
            productManager->removeProduct(produtos[idx].id);
            delete list->takeItem(idx);
        }
    });

    connect(editBtn, &QPushButton::clicked, this, [&]() {
        int idx = list->currentRow();
        if (idx >= 0 && idx < produtos.size()) {
            ProductFormDialog formDlg(produtos[idx], this);
            if (formDlg.exec() == QDialog::Accepted) {
                ProdutoFull produto = formDlg.getProduct();
                productManager->updateProduct(produtos[idx].id, produto);
                list->currentItem()->setText(QString("%1 [id:%2] x%3 — %4€")
                    .arg(produto.nome)
                    .arg(produto.id)
                    .arg(produto.quantidade)
                    .arg(QString::number(produto.preco, 'f', 2)));
            }
        }
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void MainWindow::updateAdminUI()
{
    if (editProductsButton) editProductsButton->setVisible(isAdmin);
    // update admin button text/behavior
    if (adminButton) {
        QObject::disconnect(adminButton, nullptr, nullptr, nullptr);
        if (isAdmin) {
            adminButton->setText("Logout");
            connect(adminButton, &QPushButton::clicked, this, &MainWindow::logoutAdmin);
        } else {
            adminButton->setText("Admin");
            connect(adminButton, &QPushButton::clicked, this, &MainWindow::solicitarAdmin);
        }
    }
}

MainWindow::~MainWindow(){}

// Methods for saving and loading products moved to ProductManager class

void MainWindow::logoutAdmin()
{
    isAdmin = false;
    updateAdminUI();
    QMessageBox::information(this, "Logout", "Sessão de admin terminada.");
}