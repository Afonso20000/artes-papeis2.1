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
    header->setStyleSheet("background-color: #1e1e1e; color: #BDB3A3; padding: 7px; font-size: 14px; letter-spacing: 1.1px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QHBoxLayout* navLayout = new QHBoxLayout();
    QLineEdit* searchBar = new QLineEdit();
    searchBar->setPlaceholderText("Buscar");
    searchBar->setFixedWidth(210);
    searchBar->setStyleSheet("QLineEdit { background-color: #363636; color: #ffffff; border: 1px solid #404040; padding: 5px; border-radius: 4px; }"
                           "QLineEdit:focus { border-color: #4CAF50; }");

    QLabel* logo = new QLabel("<b>Loja Artesanatos</b>");
    logo->setStyleSheet("font-size: 24px; color: #4CAF50;");

    carrinhoIconLabel = new QLabel("🛒 Carrinho (0)");
    carrinhoIconLabel->setStyleSheet("color: #4CAF50; font-size: 15px; text-decoration: underline;");
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
    carrinhoPage->setStyleSheet("QWidget { background-color: #2b2b2b; } QLabel { color: #ffffff; }");

    // --------- Página BLOG -------------
    blogPage = new QWidget;
    QVBoxLayout* blogLayout = new QVBoxLayout(blogPage);
    QLabel* blogTitle = new QLabel("Blog Artesanal");
    blogTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #4CAF50; margin: 14px;");
    blogLayout->addWidget(blogTitle);
    QTextEdit* blogContent = new QTextEdit();
    blogContent->setText("Bem-vindo ao nosso blog!\n\nAqui partilhamos novidades e dicas sobre artesanato.");
    blogContent->setReadOnly(true);
    blogContent->setStyleSheet("background-color: #363636; color: #ffffff; font-size: 15px; border: 1px solid #404040; border-radius: 4px; padding: 10px;");
    blogLayout->addWidget(blogContent);

    // --------- Página SOBRE -------------
    sobrePage = new QWidget;
    QVBoxLayout* sobreLayout = new QVBoxLayout(sobrePage);
    QLabel* sobreTitle = new QLabel("Sobre a Loja");
    sobreTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #4CAF50; margin: 14px;");
    sobreLayout->addWidget(sobreTitle);
    QLabel* sobreContent = new QLabel("Loja dedicada ao artesanato local!\nFundada em 2025.");
    sobreContent->setStyleSheet("color: #ffffff; font-size: 16px;");
    sobreContent->setAlignment(Qt::AlignCenter);
    sobreLayout->addWidget(sobreContent);

    // --------- Página INÍCIO ------------
    inicioPage = new QWidget;
    QVBoxLayout* inicioLayout = new QVBoxLayout(inicioPage);
    QLabel* inicioTitle = new QLabel("Bem-vindo à Loja de Artesanatos!");
    inicioTitle->setStyleSheet("font-size: 24px; color: #4CAF50; margin: 20px;");
    inicioLayout->addWidget(inicioTitle);
    QLabel* destaqueLbl = new QLabel("Aproveite as nossas coleções exclusivas.", inicioPage);
    destaqueLbl->setStyleSheet("color: #ffffff; font-size: 16px;");
    inicioLayout->addWidget(destaqueLbl);

    // --------- Página CONTATO -----------
    contatoPage = new QWidget;
    QVBoxLayout* contatoLayout = new QVBoxLayout(contatoPage);
    QLabel* contatoTitle = new QLabel("Contacte-nos");
    contatoTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #4CAF50;");
    contatoLayout->addWidget(contatoTitle);
    QLabel* contatoContent = new QLabel("Email: artes@loja.com\nTelemóvel: 999-999-999");
    contatoContent->setStyleSheet("color: #ffffff; font-size: 16px;");
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
    footer->setStyleSheet("color: #9e9e9e; font-size: 13px; margin-top: 27px;");
    footer->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footer);

    central->setLayout(mainLayout);
    setCentralWidget(central);
    setStyleSheet("QMainWindow { background-color: #2b2b2b; color: #ffffff; }"
                "QLabel { color: #ffffff; }"
                "QTextEdit { background-color: #363636; color: #ffffff; border: 1px solid #404040; border-radius: 4px; }"
                "QScrollArea { background-color: #2b2b2b; border: none; }"
                "QScrollBar:vertical { background-color: #363636; width: 12px; margin: 0; }"
                "QScrollBar::handle:vertical { background-color: #4a4a4a; min-height: 20px; border-radius: 6px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
                "QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical { height: 0; }");
    resize(1200, 790);

    QHBoxLayout* menuLayout = new QHBoxLayout();
    QStringList labels = {"Início", "Loja", "Carrinho", "Blog", "Sobre", "Contato"};
    for (int i = 0; i < labels.size(); ++i) {
        QPushButton* btn = new QPushButton(labels[i]);
        btn->setStyleSheet(R"(
            QPushButton {
                background: none;
                border: none;
                color: #ffffff;
                font-size: 17px;
                font-weight: bold;
                padding: 8px 23px;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #4a4a4a;
                color: #4CAF50;
                cursor: pointer;
            }
            QPushButton:pressed {
                background-color: #363636;
                color: #4CAF50;
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
            border: 1px solid #ff5252;
            color: #ff5252;
            font-size: 14px;
            padding: 6px 12px;
            border-radius: 6px;
        }
        QPushButton:hover { 
            background-color: #ff5252;
            color: #ffffff;
        }
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

void MainWindow::adicionarAoCarrinho(const QString& id) {
    carrinho[id] = carrinho.value(id, 0) + 1;
    atualizarCarrinhoIcon();
}

void MainWindow::atualizarQuantidadeCarrinho(const QString& id, int delta) {
    int novaQuant = carrinho[id] + delta;
    if (novaQuant <= 0) {
        carrinho.remove(id);
    } else {
        carrinho[id] = novaQuant;
    }
    atualizarCarrinhoIcon();
    atualizarCarrinhoPagina();
}

void MainWindow::removerDoCarrinho(const QString& id) {
    carrinho.remove(id);
    atualizarCarrinhoIcon();
    atualizarCarrinhoPagina();
}

void MainWindow::atualizarCarrinhoIcon() {
    int total = 0;
    for (int quant : carrinho.values()) {
        total += quant;
    }
    carrinhoIconLabel->setText(QString("🛒 Carrinho (%1)").arg(total));
}

void MainWindow::mostrarCarrinho() {
    paginas->setCurrentWidget(carrinhoPage);
    atualizarCarrinhoPagina();
}

void MainWindow::atualizarCarrinhoPagina() {
    auto* layout = qobject_cast<QVBoxLayout*>(carrinhoPage->layout());
    QWidget* content = new QWidget(carrinhoPage);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    
    // Limpar layout existente
    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (auto w = item->widget()) w->deleteLater();
        delete item;
    }

    // Configurar o estilo do widget de conteúdo
    content->setStyleSheet("QWidget { background-color: #2b2b2b; color: #ffffff; }");

    if (carrinho.isEmpty()) {
        QLabel* vazioLbl = new QLabel("O seu carrinho está vazio.");
        vazioLbl->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold; padding: 20px;");
        contentLayout->addWidget(vazioLbl);
    } else {
        double total = 0;
        
        // Header
        QWidget* header = new QWidget(content);
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
        header->setStyleSheet("background-color: #363636; border-radius: 8px; padding: 10px; color: #ffffff;");
        
        QLabel* produtoLabel = new QLabel("Produto");
        QLabel* precoLabel = new QLabel("Preço");
        QLabel* quantLabel = new QLabel("Quantidade");
        QLabel* headerTotalLabel = new QLabel("Total");
        QLabel* acoesLabel = new QLabel("");
        
        QString headerLabelStyle = "color: #9e9e9e; font-weight: bold; font-size: 14px;";
        produtoLabel->setStyleSheet(headerLabelStyle);
        precoLabel->setStyleSheet(headerLabelStyle);
        quantLabel->setStyleSheet(headerLabelStyle);
        headerTotalLabel->setStyleSheet(headerLabelStyle);
        
        headerLayout->addWidget(produtoLabel, 3);
        headerLayout->addWidget(precoLabel, 1);
        headerLayout->addWidget(quantLabel, 2);
        headerLayout->addWidget(headerTotalLabel, 1);
        headerLayout->addWidget(acoesLabel, 1); // Espaço para botões
        contentLayout->addWidget(header);

        // Itens do carrinho
        for (auto it = carrinho.begin(); it != carrinho.end(); ++it) {
            const QString& id = it.key();
            int quantidade = it.value();
            ProdutoFull produto = productManager->getProduct(id);
            
            QWidget* itemWidget = new QWidget(content);
            QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
            itemWidget->setStyleSheet("background-color: #363636; border-radius: 8px; margin: 5px 0; padding: 10px; border: 1px solid #404040;");

            // Container para imagem e nome
            QWidget* produtoInfo = new QWidget(itemWidget);
            QHBoxLayout* produtoInfoLayout = new QHBoxLayout(produtoInfo);
            produtoInfo->setStyleSheet("border: none;");
            
            // Imagem do produto
            QLabel* imgLabel = new QLabel(produtoInfo);
            imgLabel->setFixedSize(50, 50);
            if (!produto.imagePath.isEmpty()) {
                QPixmap px(produto.imagePath);
                if (!px.isNull()) {
                    imgLabel->setPixmap(px.scaled(imgLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    imgLabel->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(produto.cor.name()));
                }
            } else {
                imgLabel->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(produto.cor.name()));
            }
            
            QLabel* nomeLabel = new QLabel(produto.nome, produtoInfo);
            nomeLabel->setStyleSheet("font-weight: bold; color: #ffffff; font-size: 14px;");
            
            produtoInfoLayout->addWidget(imgLabel);
            produtoInfoLayout->addWidget(nomeLabel);
            produtoInfoLayout->addStretch();

            // Preço unitário
            QLabel* precoLabel = new QLabel(QString::number(produto.preco, 'f', 2) + "€");
            precoLabel->setStyleSheet("color: #9e9e9e; font-size: 14px;");
            
            // Controles de quantidade
            QWidget* quantWidget = new QWidget(itemWidget);
            QHBoxLayout* quantLayout = new QHBoxLayout(quantWidget);
            quantWidget->setStyleSheet("border: none;");
            
            QPushButton* minusBtn = new QPushButton("−", quantWidget); // Unicode minus sign
            QPushButton* plusBtn = new QPushButton("＋", quantWidget); // Unicode full-width plus
            QLabel* quantLabel = new QLabel(QString::number(quantidade), quantWidget);
            
            minusBtn->setFixedSize(26, 26);
            plusBtn->setFixedSize(26, 26);
            QString btnStyle = "QPushButton { background-color: #4a4a4a; color: #ffffff; border: none; border-radius: 13px; font-size: 16px; font-weight: bold; } "
                             "QPushButton:hover { background-color: #5a5a5a; } "
                             "QPushButton:pressed { background-color: #666666; }";
            minusBtn->setStyleSheet(btnStyle);
            plusBtn->setStyleSheet(btnStyle);
            quantLabel->setStyleSheet("color: #ffffff; font-size: 15px; font-weight: bold;");
            quantLabel->setAlignment(Qt::AlignCenter);
            quantLabel->setMinimumWidth(30);
            
            connect(minusBtn, &QPushButton::clicked, this, [this, id](){ atualizarQuantidadeCarrinho(id, -1); });
            connect(plusBtn, &QPushButton::clicked, this, [this, id](){ atualizarQuantidadeCarrinho(id, 1); });
            
            quantLayout->addWidget(minusBtn);
            quantLayout->addWidget(quantLabel);
            quantLayout->addWidget(plusBtn);
            
            // Total do item
            double totalItem = produto.preco * quantidade;
            QLabel* totalLabel = new QLabel(QString::number(totalItem, 'f', 2) + "€");
            totalLabel->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 14px;");
            
            // Botão remover
            QPushButton* removerBtn = new QPushButton("🗑️", itemWidget);
            removerBtn->setStyleSheet("QPushButton { background: none; border: none; color: #ff5252; font-size: 16px; } "
                                    "QPushButton:hover { color: #ff8a80; }");
            connect(removerBtn, &QPushButton::clicked, this, [this, id](){ removerDoCarrinho(id); });
            
            itemLayout->addWidget(produtoInfo, 3);
            itemLayout->addWidget(precoLabel, 1);
            itemLayout->addWidget(quantWidget, 2);
            itemLayout->addWidget(totalLabel, 1);
            itemLayout->addWidget(removerBtn, 1);
            
            contentLayout->addWidget(itemWidget);
            
            total += totalItem;
        }

        // Total geral
        QWidget* totalWidget = new QWidget(content);
        QHBoxLayout* totalLayout = new QHBoxLayout(totalWidget);
        totalWidget->setStyleSheet("background-color: #1e1e1e; border-radius: 8px; padding: 20px; margin-top: 20px; border: 1px solid #404040;");
        
        QLabel* totalLabel = new QLabel("Total:", totalWidget);
        totalLabel->setStyleSheet("color: #9e9e9e; font-size: 16px;");
        QLabel* valorLabel = new QLabel(QString("%1€").arg(QString::number(total, 'f', 2)), totalWidget);
        valorLabel->setStyleSheet("font-size: 20px; color: #4CAF50; font-weight: bold;");
        
        totalLayout->addWidget(totalLabel);
        totalLayout->addStretch();
        totalLayout->addWidget(valorLabel);
        
        contentLayout->addWidget(totalWidget);
    }

    contentLayout->addStretch();
    layout->addWidget(content);
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
                // Adiciona o primeiro produto disponível como demo
                auto produtos = productManager->getAllProducts();
                if (!produtos.isEmpty()) {
                    adicionarAoCarrinho(produtos.first().id);
                    QMessageBox::information(this, "Demo", "Produto demo adicionado ao carrinho.");
                }
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
        connect(card, &ProductCard::compraProduto, this, &MainWindow::adicionarAoCarrinho);
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