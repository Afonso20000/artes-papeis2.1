#include "mainwindow.h"
#include "productcard.h"
#include "productmanager.h"
#include "productformdialog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QTabWidget>
#include <QtCore/QEvent>
#include <QEvent>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QDialog>
#include <QFormLayout>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QUuid>
#include <QRegularExpression>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), carrinhoIconLabel(nullptr), adminPage(nullptr), productManager(new ProductManager(this))
{
    // Create default admin account if it doesn't exist
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists()) dataDir.mkpath(".");
    QString usersPath = dataDir.filePath("users.json");

    if (!QFile::exists(usersPath)) {
        // Criar conta admin com privilégios
        QJsonObject root;
        QJsonObject adminObj;
        
        // Gerar salt e hash para a senha padrão
        QString salt = QUuid::createUuid().toString();
        QByteArray h = QCryptographicHash::hash((salt + "admin123").toUtf8(), QCryptographicHash::Sha256);
        
        adminObj["salt"] = salt;
        adminObj["hash"] = QString(h.toHex());
        adminObj["fullName"] = "Administrator";
        adminObj["email"] = "admin@artepapeis.com";
        adminObj["phone"] = "";
        adminObj["nif"] = "";
        adminObj["isAdmin"] = true;  // Explicitamente definir como admin
        
        root["admin"] = adminObj;

        // Salvar no arquivo
        QJsonDocument outDoc(root);
        QFile f(usersPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(outDoc.toJson());
            f.close();
        }
    }

    QColor cBlack("#0E141C"), pBlue("#314B6E"), rackley("#607EA2"), weldon("#8197AC"), sPink("#BDB3A3");

    QWidget* central = new QWidget(this);
    mainLayout = new QVBoxLayout(central);

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

    carrinhoIconLabel = new ClickableLabel(this, "🛒 Carrinho (0)");
    carrinhoIconLabel->setStyleSheet("color: #4CAF50; font-size: 15px; text-decoration: underline;");
    carrinhoIconLabel->setCursor(Qt::PointingHandCursor);
    connect(carrinhoIconLabel, &ClickableLabel::clicked, this, &MainWindow::mostrarCarrinho);

    // Criar botão de login
    loginButton = new QPushButton("Log In", this);
    loginButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 5px 15px;
            border-radius: 4px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
    )");
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::abrirLogin);

    navLayout->addWidget(searchBar, 0);
    navLayout->addStretch(1);
    navLayout->addWidget(logo, 0, Qt::AlignCenter);
    navLayout->addStretch(1);
    // Carrinho será adicionado apenas se não for admin
    if (!isAdmin) {
        navLayout->addWidget(carrinhoIconLabel, 0);
    }
    
    // Add user name label (initially hidden)
    userNameLabel = new QLabel(this);
    userNameLabel->setStyleSheet(R"(
        QLabel {
            color: #4CAF50;
            font-size: 15px;
            padding: 5px 10px;
            border-radius: 4px;
            cursor: pointer;
        }
        QLabel:hover {
            background-color: #45a049;
            color: white;
        }
    )");
    userNameLabel->hide();
    userNameLabel->setCursor(Qt::PointingHandCursor);
    userNameLabel->installEventFilter(this);
    navLayout->addWidget(userNameLabel, 0);
    
    navLayout->addWidget(loginButton, 0);

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

    // Reserva timer: periodic check to release expired reservations
    reservationTimer = new QTimer(this);
    connect(reservationTimer, &QTimer::timeout, this, &MainWindow::checkReservations);
    reservationTimer->start(60 * 1000); // checar a cada minuto
    // Uma verificação imediata inicial
    checkReservations();

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
    QStringList labels = {"Início", "Loja", "Sobre", "Contato"}; // Removido "Carrinho"
    menuButtons.clear(); // Limpar a lista de botões
    for (int i = 0; i < labels.size(); ++i) {
        QPushButton* btn = new QPushButton(labels[i]);
        menuButtons.append(btn); // Armazenar referência ao botão
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
        
        // Índice para o stacked widget
        int pageIndex = (i >= 2) ? i + 1 : i; // Ajustar índice para pular a página do carrinho
        
        connect(btn, &QPushButton::clicked, this, [this, pageIndex]() {
            paginas->setCurrentIndex(pageIndex);
        });
        menuLayout->addWidget(btn);
    }
    mainLayout->insertLayout(3, menuLayout);

    paginas->setCurrentIndex(0);
}

void MainWindow::abrirSobre()   { paginas->setCurrentWidget(sobrePage);       }
void MainWindow::abrirLoja()    { paginas->setCurrentWidget(lojaPage);        }
void MainWindow::abrirInicio()  { paginas->setCurrentWidget(inicioPage);      }
void MainWindow::abrirContato() { paginas->setCurrentWidget(contatoPage);     }

void MainWindow::adicionarAoCarrinho(const QString& id) {
    // Try to reserve 1 unit for the user (reserve-at-add model)
    ProdutoFull produto = productManager->getProduct(id);
    if (produto.id.isEmpty()) {
        QMessageBox::warning(this, "Produto inválido", "Produto não encontrado.");
        return;
    }

    // Reserva por defeito 15 minutos
    QDateTime expires = QDateTime::currentDateTime().addSecs(15 * 60);
    bool ok = productManager->reserveProduct(id, 1, expires);
    if (!ok) {
        QMessageBox::warning(this, "Sem estoque", QString("%1 está esgotado ou não há unidades suficientes disponíveis.").arg(produto.nome));
        return;
    }

    // Add to cart (reservation holds the unit)
    carrinho[id] = carrinho.value(id, 0) + 1;
    atualizarCarrinhoIcon();
    atualizarCarrinhoPagina();

    // Atualizar UI dos cards para refletir stock disponível
    refreshLojaProducts();
}

void MainWindow::atualizarQuantidadeCarrinho(const QString& id, int delta) {
    int atual = carrinho.value(id, 0);
    int novaQuant = atual + delta;
    ProdutoFull produto = productManager->getProduct(id);
    if (produto.id.isEmpty()) {
        QMessageBox::warning(this, "Produto inválido", "Produto não encontrado.");
        return;
    }

    if (delta > 0) {
        // try to reserve additional units
        QDateTime expires = QDateTime::currentDateTime().addSecs(15 * 60);
        bool ok = productManager->reserveProduct(id, delta, expires);
        if (!ok) {
            int avail = productManager->getAvailableStock(id);
            QMessageBox::warning(this, "Sem estoque suficiente", QString("Só restam %1 unidades de %2.").arg(avail).arg(produto.nome));
            return;
        }
        carrinho[id] = novaQuant;
    } else if (delta < 0) {
        // decreasing quantity in cart: return items to stock
        int toReturn = -delta;
        // release reservations we previously created
        productManager->releaseReservation(id, toReturn);
        if (novaQuant <= 0) carrinho.remove(id); else carrinho[id] = novaQuant;
    }

    atualizarCarrinhoIcon();
    atualizarCarrinhoPagina();
}

void MainWindow::removerDoCarrinho(const QString& id) {
    int quant = carrinho.value(id, 0);
    if (quant > 0) {
        ProdutoFull produto = productManager->getProduct(id);
        if (!produto.id.isEmpty()) {
            // release reservations for all units in cart
            productManager->releaseReservation(id, quant);
        }
    }
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
            
            QPushButton* minusBtn = new QPushButton("-", quantWidget);
            QPushButton* plusBtn = new QPushButton("+", quantWidget);
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

        // Adicionar botão "Finalizar Compra"
        QPushButton* finalizarBtn = new QPushButton("Finalizar Compra", content);
        finalizarBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                padding: 12px 24px;
                border-radius: 6px;
                font-size: 16px;
                font-weight: bold;
                margin-top: 20px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:pressed {
                background-color: #3d8b40;
            }
        )");
        connect(finalizarBtn, &QPushButton::clicked, this, &MainWindow::finalizarCompra);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(finalizarBtn);
        btnLayout->addStretch();
        contentLayout->addLayout(btnLayout);
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
    // Verificar credenciais no users.json
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    QString usersPath = dataDir.filePath("users.json");

    bool isFirstUser = false;
    if (!QFile::exists(usersPath)) {
        isFirstUser = true;
        if (!dataDir.exists()) {
            dataDir.mkpath(".");
        }
        // Criar o primeiro usuário como admin
        QString err;
        if (!salvarUsuario("admin", "admin123", "Administrator", "admin@artepapeis.com", "", "", err)) {
            QMessageBox::warning(this, "Erro", QString("Não foi possível criar usuário admin: %1").arg(err));
            return;
        }
    }

    // Se acabamos de criar o usuário admin e a senha fornecida é a padrão
    if (isFirstUser && senha == "admin123") {
        isAdmin = true;
        loggedInUser = "admin";
        updateAdminUI();
        QMessageBox::information(this, "Acesso Admin", "Login com senha padrão. Recomendamos alterar a senha.");
        setupAdminPage();
        paginas->setCurrentWidget(adminPage);
        return;
    }

    // Verificar credenciais nos usuários existentes
    QFile file(usersPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erro", "Não foi possível ler os dados dos usuários.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::warning(this, "Erro", "Arquivo de usuários corrompido.");
        return;
    }

    // Verificar todas as contas de admin
    bool foundAdmin = false;
    QJsonObject users = doc.object();
    for (auto it = users.begin(); it != users.end(); ++it) {
        QJsonObject userObj = it.value().toObject();
        if (userObj["isAdmin"].toBool()) {
            QString salt = userObj["salt"].toString();
            QString expected = userObj["hash"].toString();
            QByteArray h = QCryptographicHash::hash((salt + senha).toUtf8(), QCryptographicHash::Sha256);
            if (QString(h.toHex()) == expected) {
                foundAdmin = true;
                loggedInUser = it.key();
                break;
            }
        }
    }

    if (foundAdmin) {
        isAdmin = true;
        updateAdminUI();
        QMessageBox::information(this, "Acesso Admin", "Login efetuado com sucesso.");
        setupAdminPage();
        paginas->setCurrentWidget(adminPage);
    } else {
        QMessageBox::warning(this, "Acesso Negado", "Usuário não é administrador ou senha incorreta.");
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
        // update card with current available stock (takes reservations into account)
        card->setAvailableStock(productManager->getAvailableStock(pf.id));
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
    // Mostrar/esconder botão de editar produtos
    if (editProductsButton) {
        editProductsButton->setVisible(isAdmin);
    }

    // Atualizar visibilidade do carrinho
    if (carrinhoIconLabel) {
        carrinhoIconLabel->setVisible(!isAdmin);
    }
    
    // Se for admin, configurar a página administrativa
    if (isAdmin) {
        if (!adminPage) {
            setupAdminPage();
            paginas->addWidget(adminPage);
        }

        // Procurar o layout do menu e verificar se já tem botão Admin
        QHBoxLayout* menuLayout = nullptr;
        bool hasAdminButton = false;
        
        // Procurar o layout do menu
        for (int i = 0; i < mainLayout->count(); i++) {
            QLayoutItem* item = mainLayout->itemAt(i);
            if (QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(item->layout())) {
                // Verificar se é o layout do menu procurando pelos botões
                QLayoutItem* firstItem = hLayout->itemAt(0);
                if (firstItem && firstItem->widget()) {
                    QPushButton* btn = qobject_cast<QPushButton*>(firstItem->widget());
                    if (btn && btn->text() == "Início") {
                        menuLayout = hLayout;
                        // Verificar se já existe o botão Admin
                        for (int j = 0; j < hLayout->count(); j++) {
                            QLayoutItem* menuItem = hLayout->itemAt(j);
                            if (QPushButton* menuBtn = qobject_cast<QPushButton*>(menuItem->widget())) {
                                if (menuBtn->text() == "Admin") {
                                    hasAdminButton = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        
        if (menuLayout && !hasAdminButton) {
            QPushButton* adminBtn = new QPushButton("Admin");
            adminBtn->setStyleSheet(R"(
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
            connect(adminBtn, &QPushButton::clicked, this, [this]() {
                if (adminPage) {
                    paginas->setCurrentWidget(adminPage);
                }
            });
            menuLayout->addWidget(adminBtn);
        }
        
        // Forçar atualização da página administrativa
        if (paginas->currentWidget() != adminPage) {
            paginas->setCurrentWidget(adminPage);
        }
    } else {
        // Se não for admin, remover o botão Admin se existir
        for (int i = 0; i < mainLayout->count(); i++) {
            QLayoutItem* item = mainLayout->itemAt(i);
            if (QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(item->layout())) {
                for (int j = 0; j < hLayout->count(); j++) {
                    QLayoutItem* menuItem = hLayout->itemAt(j);
                    if (QPushButton* btn = qobject_cast<QPushButton*>(menuItem->widget())) {
                        if (btn->text() == "Admin") {
                            btn->deleteLater();
                            break;
                        }
                    }
                }
            }
        }
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == userNameLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            setupUserMenu();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setupUserMenu()
{
    QMenu* menu = new QMenu(this);
    menu->setStyleSheet(R"(
        QMenu {
            background-color: #2b2b2b;
            border: 1px solid #404040;
            padding: 5px;
        }
        QMenu::item {
            color: #ffffff;
            padding: 8px 20px;
        }
        QMenu::item:selected {
            background-color: #4CAF50;
        }
    )");

    if (!isAdmin) {
        QAction* historicoAction = menu->addAction("Minhas Encomendas");
        connect(historicoAction, &QAction::triggered, this, &MainWindow::mostrarEncomendas);
    }
    QAction* logoutAction = menu->addAction("Logout");
    connect(logoutAction, &QAction::triggered, this, &MainWindow::logoutUser);

    menu->popup(userNameLabel->mapToGlobal(QPoint(0, userNameLabel->height())));
}

void MainWindow::mostrarEncomendas()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Minhas Encomendas");
    dialog->resize(800, 600);
    dialog->setStyleSheet("QDialog { background-color: #2b2b2b; color: #ffffff; }");

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // Carregar todas as encomendas
    QVector<Order> orders = carregarEncomendas();

    // Filtrar apenas as encomendas do usuário atual
    QVector<Order> userOrders;
    for (const Order& order : orders) {
        if (order.userId == loggedInUser) {
            userOrders.append(order);
        }
    }

    if (userOrders.isEmpty()) {
        QLabel* emptyLabel = new QLabel("Você ainda não tem encomendas.", dialog);
        emptyLabel->setStyleSheet("color: #ffffff; font-size: 16px; padding: 20px;");
        layout->addWidget(emptyLabel);
    } else {
        QScrollArea* scrollArea = new QScrollArea(dialog);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: none; }");

        QWidget* contentWidget = new QWidget(scrollArea);
        QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);

        // Ordenar encomendas por data (mais recente primeiro)
        std::sort(userOrders.begin(), userOrders.end(), 
                 [](const Order& a, const Order& b) { return a.orderDate > b.orderDate; });

        for (const Order& order : userOrders) {
            QWidget* orderWidget = new QWidget(contentWidget);
            orderWidget->setStyleSheet(
                "QWidget { background-color: #363636; border-radius: 8px; margin: 5px; padding: 15px; }"
                "QLabel { color: #ffffff; }"
            );
            QVBoxLayout* orderLayout = new QVBoxLayout(orderWidget);

            // Cabeçalho da encomenda
            QLabel* headerLabel = new QLabel(QString("Encomenda: %1").arg(order.orderId));
            headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4CAF50;");
            orderLayout->addWidget(headerLabel);

            QLabel* dateLabel = new QLabel(QString("Data: %1")
                .arg(order.orderDate.toString("dd/MM/yyyy HH:mm")));
            dateLabel->setStyleSheet("color: #9e9e9e;");
            orderLayout->addWidget(dateLabel);

            // Lista de itens
            for (const OrderItem& item : order.items) {
                QLabel* itemLabel = new QLabel(QString("%1x %2 - %3€")
                    .arg(item.quantity)
                    .arg(item.productName)
                    .arg(item.price * item.quantity, 0, 'f', 2));
                orderLayout->addWidget(itemLabel);
            }

            // Total
            QLabel* totalLabel = new QLabel(QString("Total: %1€")
                .arg(order.total, 0, 'f', 2));
            totalLabel->setStyleSheet("font-weight: bold; color: #4CAF50; margin-top: 10px;");
            orderLayout->addWidget(totalLabel);

            contentLayout->addWidget(orderWidget);
        }

        contentLayout->addStretch();
        scrollArea->setWidget(contentWidget);
        layout->addWidget(scrollArea);
    }

    QPushButton* closeButton = new QPushButton("Fechar", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(closeButton, 0, Qt::AlignRight);
    dialog->exec();
    delete dialog;
}

MainWindow::~MainWindow(){}

// Methods for saving and loading products moved to ProductManager class

void MainWindow::logoutAdmin()
{
    isAdmin = false;
    updateAdminUI();
    QMessageBox::information(this, "Logout", "Sessão de admin terminada.");
}

void MainWindow::abrirLogin()
{
    // Criar a página de login na primeira vez que for chamada
    if (!loginPage) {
        loginPage = new QWidget;
        loginPage->setStyleSheet("QWidget { background-color: #2b2b2b; color: #ffffff; }");
        QVBoxLayout* layout = new QVBoxLayout(loginPage);

        QLabel* title = new QLabel("Entrar na sua conta", loginPage);
        title->setStyleSheet("font-size: 22px; font-weight: bold; color: #4CAF50; margin: 12px;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);

        // Form container
        QWidget* form = new QWidget(loginPage);
        QVBoxLayout* formLayout = new QVBoxLayout(form);
        formLayout->setContentsMargins(40, 10, 40, 10);

        QLabel* userLbl = new QLabel("Username", form);
        userLbl->setStyleSheet("color: #ffffff; font-size: 14px; margin-top: 8px;");
        loginUsernameEdit = new QLineEdit(form);
        loginUsernameEdit->setPlaceholderText("username");
        loginUsernameEdit->setStyleSheet("QLineEdit { background-color: #3a3a3a; color: #ffffff; padding: 8px; border-radius: 6px; }");

        QLabel* passLbl = new QLabel("Password", form);
        passLbl->setStyleSheet("color: #ffffff; font-size: 14px; margin-top: 8px;");
        loginPasswordEdit = new QLineEdit(form);
        loginPasswordEdit->setPlaceholderText("password");
        loginPasswordEdit->setEchoMode(QLineEdit::Password);
        loginPasswordEdit->setStyleSheet("QLineEdit { background-color: #3a3a3a; color: #ffffff; padding: 8px; border-radius: 6px; }");

        formLayout->addWidget(userLbl);
        formLayout->addWidget(loginUsernameEdit);
        formLayout->addWidget(passLbl);
        formLayout->addWidget(loginPasswordEdit);
        form->setLayout(formLayout);

        layout->addWidget(form);

        // Entrar button immediately below the password
        QPushButton* signInBtn = new QPushButton("Entrar", loginPage);
        signInBtn->setFixedWidth(160);
        signInBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #4CAF50;
                color: white;
                border: none;
                padding: 10px 16px;
                border-radius: 6px;
                font-size: 15px;
            }
            QPushButton:hover { background-color: #45a049; }
        )");
        connect(signInBtn, &QPushButton::clicked, this, &MainWindow::handleLogin);
        // Pressing Enter in password field triggers login
        connect(loginPasswordEdit, &QLineEdit::returnPressed, signInBtn, &QPushButton::click);
        QWidget* btnWrap = new QWidget(loginPage);
        QHBoxLayout* btnRow = new QHBoxLayout(btnWrap);
        btnRow->addStretch(1);
        btnRow->addWidget(signInBtn);
        btnRow->addStretch(1);
        btnWrap->setLayout(btnRow);
        layout->addWidget(btnWrap);

        // Small "Criar Conta" button below Entrar
        QPushButton* createBtn = new QPushButton("Criar Conta", loginPage);
        createBtn->setFlat(true);
        createBtn->setStyleSheet("QPushButton { color: #9e9e9e; text-decoration: underline; border: none; } QPushButton:hover { color: #ffffff; }");
        connect(createBtn, &QPushButton::clicked, this, &MainWindow::criarConta);
        QWidget* createWrap = new QWidget(loginPage);
        QHBoxLayout* createRow = new QHBoxLayout(createWrap);
        createRow->addStretch(1);
        createRow->addWidget(createBtn);
        createRow->addStretch(1);
        createWrap->setLayout(createRow);
        layout->addWidget(createWrap);

        // Push other content to take space so the buttons remain near the form
        layout->addStretch(1);
        loginPage->setLayout(layout);
        paginas->addWidget(loginPage);
    }

    paginas->setCurrentWidget(loginPage);
}

void MainWindow::handleLogin()
{
    QString user = loginUsernameEdit ? loginUsernameEdit->text().trimmed() : QString();
    QString pass = loginPasswordEdit ? loginPasswordEdit->text() : QString();

    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Login", "Por favor preencha o username e a password.");
        return;
    }

    // Verificar se é o admin com as credenciais padrão
    if (user == "admin" && pass == "admin123") {
        loggedInUser = user;
        isAdmin = true;
        
        // Criar/atualizar usuário admin no arquivo se necessário
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dataDir(dataPath + "/artes-papeis");
        QString usersPath = dataDir.filePath("users.json");
        
        if (!dataDir.exists()) {
            dataDir.mkpath(".");
        }
        
        QJsonObject root;
        if (QFile::exists(usersPath)) {
            QFile readFile(usersPath);
            if (readFile.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(readFile.readAll());
                if (doc.isObject()) {
                    root = doc.object();
                }
                readFile.close();
            }
        }
        
        // Criar ou atualizar usuário admin
        QJsonObject adminObj;
        QString salt = QUuid::createUuid().toString();
        QByteArray h = QCryptographicHash::hash((salt + pass).toUtf8(), QCryptographicHash::Sha256);
        
        adminObj["salt"] = salt;
        adminObj["hash"] = QString(h.toHex());
        adminObj["fullName"] = "Administrator";
        adminObj["email"] = "admin@artepapeis.com";
        adminObj["phone"] = "";
        adminObj["nif"] = "";
        adminObj["isAdmin"] = true;
        
        root["admin"] = adminObj;
        
        QFile writeFile(usersPath);
        if (writeFile.open(QIODevice::WriteOnly)) {
            writeFile.write(QJsonDocument(root).toJson());
            writeFile.close();
        }
        
        // Configurar interface administrativa
        if (!adminPage) {
            setupAdminPage();
        }
        
        // Atualizar UI
        loginButton->hide();
        userNameLabel->setText("Olá, Administrator");
        userNameLabel->show();
        updateAdminUI();
        
        // Mostrar página administrativa
        paginas->setCurrentWidget(adminPage);
        QMessageBox::information(this, "Login", "Login de administrador efetuado com sucesso.");
        return;
    }
    
    // Para outros usuários, verificar credenciais normalmente
    if (validarCredenciais(user, pass)) {
        loggedInUser = user;
        
        // Verificar se é uma conta de administrador no arquivo de usuários
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dataPath + "/artes-papeis/users.json");
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                QJsonObject userObj = doc.object().value(user).toObject();
                isAdmin = userObj["isAdmin"].toBool();
            }
            f.close();
        }
        
        // Atualizar UI
        loginButton->hide();
        userNameLabel->setText("Olá, " + user);
        userNameLabel->show();
        updateAdminUI();
        
        if (isAdmin) {
            if (!adminPage) {
                setupAdminPage();
            }
            paginas->setCurrentWidget(adminPage);
            QMessageBox::information(this, "Login", "Login de administrador efetuado com sucesso.");
        } else {
            paginas->setCurrentWidget(lojaPage);
            QMessageBox::information(this, "Login", "Login efetuado com sucesso.");
        }
    } else {
        QMessageBox::warning(this, "Login", "Username ou password inválidos.");
    }
}

void MainWindow::logoutUser()
{
    loggedInUser.clear();
    
    // Handle admin logout if necessary
    if (isAdmin) {
        isAdmin = false;
        updateAdminUI();
    }
    
    // Hide username label and show login button
    userNameLabel->hide();
    loginButton->show();
    
    // Reset login button state
    QObject::disconnect(loginButton, nullptr, nullptr, nullptr);
    loginButton->setText("Log In");
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::abrirLogin);
    QMessageBox::information(this, "Logout", "Sessão terminada.");
}

void MainWindow::criarConta()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Criar Conta");
    QVBoxLayout* main = new QVBoxLayout(&dlg);
    QFormLayout* form = new QFormLayout();

    QLineEdit* userEdit = new QLineEdit(&dlg);
    QLineEdit* passEdit = new QLineEdit(&dlg);
    QLineEdit* passConfirm = new QLineEdit(&dlg);
    passEdit->setEchoMode(QLineEdit::Password);
    passConfirm->setEchoMode(QLineEdit::Password);

    form->addRow("Username:", userEdit);
    form->addRow("Password:", passEdit);
    form->addRow("Confirmar Password:", passConfirm);
    
    // Additional requested fields
    QLineEdit* fullNameEdit = new QLineEdit(&dlg);
    QLineEdit* emailEdit = new QLineEdit(&dlg);
    QLineEdit* phoneEdit = new QLineEdit(&dlg);
    QLineEdit* nifEdit = new QLineEdit(&dlg);
    phoneEdit->setPlaceholderText("(opcional)");
    nifEdit->setPlaceholderText("9 dígitos, sem espaços");
    form->addRow("Nome completo:", fullNameEdit);
    form->addRow("Email:", emailEdit);
    form->addRow("Telemóvel:", phoneEdit);
    form->addRow("NIF:", nifEdit);
    main->addLayout(form);

    QHBoxLayout* btns = new QHBoxLayout();
    btns->addStretch(1);
    QPushButton* ok = new QPushButton("Criar", &dlg);
    QPushButton* cancel = new QPushButton("Cancelar", &dlg);
    btns->addWidget(ok);
    btns->addWidget(cancel);
    main->addLayout(btns);

    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &dlg, [&]() {
        QString u = userEdit->text().trimmed();
        QString p = passEdit->text();
        QString pc = passConfirm->text();
        QString full = fullNameEdit->text().trimmed();
        QString email = emailEdit->text().trimmed();
        QString phone = phoneEdit->text().trimmed();
        QString nif = nifEdit->text().trimmed();
        if (u.isEmpty() || p.isEmpty()) {
            QMessageBox::warning(&dlg, "Criar Conta", "Username e password não podem estar vazios.");
            return;
        }
        if (p != pc) {
            QMessageBox::warning(&dlg, "Criar Conta", "Passwords não coincidem.");
            return;
        }
        // Basic validations
        if (email.isEmpty() || !email.contains('@') || !email.contains('.')) {
            QMessageBox::warning(&dlg, "Criar Conta", "Por favor insira um email válido.");
            return;
        }
        QRegularExpression nifRx("^\\d{9}$");
        if (!nifRx.match(nif).hasMatch()) {
            QMessageBox::warning(&dlg, "Criar Conta", "NIF inválido. Deve conter 9 dígitos.");
            return;
        }
        QString err;
        if (!salvarUsuario(u, p, full, email, phone, nif, err)) {
            QMessageBox::warning(&dlg, "Criar Conta", QString("Não foi possível criar conta: %1").arg(err));
            return;
        }
        QMessageBox::information(&dlg, "Criar Conta", "Conta criada com sucesso. Pode agora entrar.");
        dlg.accept();
    });

    dlg.exec();
}

bool MainWindow::salvarUsuario(const QString& username, const QString& password,
                              const QString& fullName, const QString& email,
                              const QString& phone, const QString& nif,
                              QString& outError)
{
    outError.clear();
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists() && !dataDir.mkpath(".")) {
        outError = "Não foi possível criar diretório de dados.";
        return false;
    }
    QString usersPath = dataDir.filePath("users.json");

    QJsonObject root;
    if (QFile::exists(usersPath)) {
        QFile f(usersPath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray c = f.readAll();
            f.close();
            QJsonDocument jd = QJsonDocument::fromJson(c);
            if (jd.isObject()) root = jd.object();
        }
    }

    if (root.contains(username)) {
        outError = "Username já existe.";
        return false;
    }

    QString salt = QUuid::createUuid().toString();
    QByteArray h = QCryptographicHash::hash((salt + password).toUtf8(), QCryptographicHash::Sha256);
    QJsonObject userObj;
    userObj["salt"] = salt;
    userObj["hash"] = QString(h.toHex());
    // Additional profile fields
    userObj["fullName"] = fullName;
    userObj["email"] = email;
    userObj["phone"] = phone;
    userObj["nif"] = nif;
    // Definir como admin apenas se for o primeiro usuário (admin padrão)
    bool isFirstUser = root.isEmpty();
    userObj["isAdmin"] = isFirstUser;
    root[username] = userObj;

    QJsonDocument outDoc(root);
    QFile f(usersPath);
    if (!f.open(QIODevice::WriteOnly)) {
        outError = "Não foi possível escrever ficheiro de utilizadores.";
        return false;
    }
    f.write(outDoc.toJson());
    f.close();
    return true;
}

bool MainWindow::validarCredenciais(const QString& username, const QString& password)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    QString usersPath = dataDir.filePath("users.json");
    
    // Se o arquivo não existe e as credenciais são do admin padrão
    if (!QFile::exists(usersPath)) {
        if (username == "admin" && password == "admin123") {
            return true;
        }
        return false;
    }
    
    QFile f(usersPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray c = f.readAll();
    f.close();
    
    QJsonDocument jd = QJsonDocument::fromJson(c);
    if (!jd.isObject()) return false;
    
    QJsonObject root = jd.object();
    if (!root.contains(username)) return false;
    
    QJsonObject userObj = root.value(username).toObject();
    QString salt = userObj.value("salt").toString();
    QString expected = userObj.value("hash").toString();
    QByteArray h = QCryptographicHash::hash((salt + password).toUtf8(), QCryptographicHash::Sha256);
    
    // Verificar senha
    return (QString(h.toHex()) == expected);
}

void MainWindow::checkReservations()
{
    // Release expired reservations and refresh UI
    productManager->releaseExpiredReservations();
    refreshLojaProducts();
    atualizarCarrinhoIcon();

    // If admin logged in, optionally show low-stock alerts
    if (isAdmin) {
        auto low = productManager->getLowStockProducts();
        for (const auto &p : low) notifyAdminLowStock(p);
    }
}

void MainWindow::showLowStockPanel()
{
    QVector<ProdutoFull> low = productManager->getLowStockProducts();
    QDialog dlg(this);
    dlg.setWindowTitle("Produtos com estoque baixo");
    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    for (const auto &p : low) {
        QLabel* lbl = new QLabel(QString("%1 [id:%2] — Disponível: %3 — Limite: %4")
                                 .arg(p.nome).arg(p.id).arg(productManager->getAvailableStock(p.id)).arg(p.lowThreshold));
        layout->addWidget(lbl);
    }
    QPushButton* close = new QPushButton("Fechar", &dlg);
    connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(close);
    dlg.exec();
}

void MainWindow::notifyAdminLowStock(const ProdutoFull& p)
{
    QString msg = QString("Produto '%1' com baixo stock: %2 disponível (limite %3)")
                  .arg(p.nome).arg(productManager->getAvailableStock(p.id)).arg(p.lowThreshold);
    // Show an in-app message; if system tray available, use it
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon tray(this);
        tray.show();
        tray.showMessage("Alerta de Stock", msg, QSystemTrayIcon::Warning, 5000);
        // tray will be destroyed at end of scope; message still shows on most platforms
    } else {
        // fallback to message box
        QMessageBox::information(this, "Alerta de Stock", msg);
    }
}

QString MainWindow::gerarOrderId() {
    // Gerar um ID único baseado na data/hora e um número aleatório
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    // Use o último dígito dos milissegundos como parte do ID
    return QString("ORD-%1").arg(timestamp);
}

bool MainWindow::salvarEncomenda(const Order& order, QString& outError) {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists() && !dataDir.mkpath(".")) {
        outError = "Não foi possível criar diretório de dados.";
        return false;
    }
    QString ordersPath = dataDir.filePath("orders.json");

    // Carregar encomendas existentes
    QJsonArray ordersArray;
    if (QFile::exists(ordersPath)) {
        QFile f(ordersPath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray()) {
                ordersArray = doc.array();
            }
            f.close();
        }
    }

    // Criar objeto JSON para a nova encomenda
    QJsonObject orderObj;
    orderObj["orderId"] = order.orderId;
    orderObj["userId"] = order.userId;
    orderObj["userName"] = order.userName;
    orderObj["orderDate"] = order.orderDate.toString(Qt::ISODate);
    orderObj["total"] = order.total;

    QJsonArray itemsArray;
    for (const OrderItem& item : order.items) {
        QJsonObject itemObj;
        itemObj["productId"] = item.productId;
        itemObj["productName"] = item.productName;
        itemObj["quantity"] = item.quantity;
        itemObj["price"] = item.price;
        itemsArray.append(itemObj);
    }
    orderObj["items"] = itemsArray;

    // Adicionar nova encomenda ao array
    ordersArray.append(orderObj);

    // Salvar arquivo atualizado
    QFile f(ordersPath);
    if (!f.open(QIODevice::WriteOnly)) {
        outError = "Não foi possível salvar o arquivo de encomendas.";
        return false;
    }
    QJsonDocument doc(ordersArray);
    f.write(doc.toJson());
    f.close();

    return true;
}

QVector<Order> MainWindow::carregarEncomendas() {
    QVector<Order> orders;
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString ordersPath = dataPath + "/artes-papeis/orders.json";

    if (!QFile::exists(ordersPath)) return orders;

    QFile f(ordersPath);
    if (!f.open(QIODevice::ReadOnly)) return orders;

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return orders;

    QJsonArray ordersArray = doc.array();
    for (const QJsonValue& val : ordersArray) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        Order order;
        order.orderId = obj["orderId"].toString();
        order.userId = obj["userId"].toString();
        order.userName = obj["userName"].toString();
        order.orderDate = QDateTime::fromString(obj["orderDate"].toString(), Qt::ISODate);
        order.total = obj["total"].toDouble();

        QJsonArray itemsArray = obj["items"].toArray();
        for (const QJsonValue& itemVal : itemsArray) {
            QJsonObject itemObj = itemVal.toObject();
            OrderItem item;
            item.productId = itemObj["productId"].toString();
            item.productName = itemObj["productName"].toString();
            item.quantity = itemObj["quantity"].toInt();
            item.price = itemObj["price"].toDouble();
            order.items.append(item);
        }

        orders.append(order);
    }

    return orders;
}

void MainWindow::limparCarrinho() {
    carrinho.clear();
    atualizarCarrinhoIcon();
    atualizarCarrinhoPagina();
}

void MainWindow::setupAdminPage() {
    if (!adminPage) {
        adminPage = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(adminPage);
        
        // Título da área administrativa
        QLabel* lbl = new QLabel("Área Administrativa", adminPage);
        lbl->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50; margin-bottom: 20px;");
        layout->addWidget(lbl, 0, Qt::AlignCenter);

        // Criar o widget de tabs
        adminTabWidget = new QTabWidget(adminPage);
        adminTabWidget->setStyleSheet(R"(
            QTabWidget::pane {
                border: 1px solid #404040;
                background-color: #2b2b2b;
                padding: 15px;
            }
            QTabWidget::tab-bar {
                alignment: left;
            }
            QTabBar::tab {
                background-color: #363636;
                color: #ffffff;
                padding: 8px 20px;
                margin-right: 2px;
            }
            QTabBar::tab:hover {
                background-color: #404040;
            }
            QTabBar::tab:selected {
                background-color: #4CAF50;
            }
        )");

        // Tab de Produtos
        QWidget* produtosTab = new QWidget();
        QVBoxLayout* produtosLayout = new QVBoxLayout(produtosTab);
        QPushButton* editProdutosBtn = new QPushButton("Gerenciar Produtos", produtosTab);
        editProdutosBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #4CAF50;
                color: white;
                padding: 10px 20px;
                border: none;
                border-radius: 4px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
        )");
        connect(editProdutosBtn, &QPushButton::clicked, this, &MainWindow::showProductManager);
        produtosLayout->addWidget(editProdutosBtn);
        produtosLayout->addStretch();
        
        adminTabWidget->addTab(produtosTab, "Produtos");

        // Inicializar a aba de encomendas
        setupAdminOrdersTab();

        // Adicionar widget de tabs ao layout e configurar a página
        layout->addWidget(adminTabWidget);
        adminPage->setLayout(layout);

        // Adicionar ao stacked widget
        paginas->addWidget(adminPage);
    }
}

void MainWindow::setupAdminOrdersTab() {
    QWidget* encomendasTab = new QWidget();
    QVBoxLayout* encomendasLayout = new QVBoxLayout(encomendasTab);
    
    // Lista de encomendas
    adminOrdersList = new QListWidget(encomendasTab);
    adminOrdersList->setStyleSheet(R"(
        QListWidget {
            background-color: #363636;
            border: 1px solid #404040;
            border-radius: 4px;
            color: #ffffff;
        }
        QListWidget::item {
            padding: 10px;
            border-bottom: 1px solid #404040;
        }
        QListWidget::item:hover {
            background-color: #404040;
        }
        QListWidget::item:selected {
            background-color: #4CAF50;
        }
    )");

    // Cabeçalho explicativo
    QLabel* headerLabel = new QLabel("Duplo clique em uma encomenda para ver os detalhes", encomendasTab);
    headerLabel->setStyleSheet("color: #9e9e9e; margin-bottom: 10px;");
    encomendasLayout->addWidget(headerLabel);

    encomendasLayout->addWidget(adminOrdersList);

    // Botão de atualizar
    QPushButton* refreshBtn = new QPushButton("Atualizar Lista", encomendasTab);
    refreshBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            font-size: 14px;
            margin-top: 10px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::atualizarListaEncomendas);
    encomendasLayout->addWidget(refreshBtn);

    // Conectar o clique duplo para mostrar detalhes
    connect(adminOrdersList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString orderId = item->data(Qt::UserRole).toString();
        mostrarDetalhesEncomenda(orderId);
    });

    adminTabWidget->addTab(encomendasTab, "Encomendas");
    atualizarListaEncomendas();
}

void MainWindow::atualizarListaEncomendas() {
    if (!adminOrdersList) return;

    adminOrdersList->clear();
    QVector<Order> orders = carregarEncomendas();
    
    // Ordenar por data mais recente primeiro
    std::sort(orders.begin(), orders.end(), 
             [](const Order& a, const Order& b) { return a.orderDate > b.orderDate; });

    for (const Order& order : orders) {
        // Carregar informações do usuário
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dataPath + "/artes-papeis/users.json");
        QString userName = order.userId;
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                QJsonObject userObj = doc.object().value(order.userId).toObject();
                userName = userObj["fullName"].toString();
                if (userName.isEmpty()) userName = order.userId;
            }
            f.close();
        }

        QString itemText = QString("%1 - %2 - Cliente: %3 - Total: %4€")
            .arg(order.orderDate.toString("dd/MM/yyyy HH:mm"))
            .arg(order.orderId)
            .arg(userName)
            .arg(order.total, 0, 'f', 2);
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, order.orderId);
        adminOrdersList->addItem(item);
    }
}

void MainWindow::mostrarDetalhesEncomenda(const QString& orderId) {
    // Carregar todas as encomendas
    QVector<Order> orders = carregarEncomendas();
    
    // Encontrar a encomenda específica
    Order* targetOrder = nullptr;
    for (Order& order : orders) {
        if (order.orderId == orderId) {
            targetOrder = &order;
            break;
        }
    }
    
    if (!targetOrder) {
        QMessageBox::warning(this, "Erro", "Encomenda não encontrada.");
        return;
    }

    // Criar janela de detalhes
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Detalhes da Encomenda");
    dialog->resize(600, 500);
    dialog->setStyleSheet("QDialog { background-color: #2b2b2b; color: #ffffff; }");

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // Header com ID da encomenda
    QLabel* headerLabel = new QLabel(QString("Encomenda: %1").arg(targetOrder->orderId));
    headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #4CAF50; margin-bottom: 10px;");
    layout->addWidget(headerLabel);

    // Informações do cliente
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile f(dataPath + "/artes-papeis/users.json");
    QJsonObject userData;
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
            userData = doc.object().value(targetOrder->userId).toObject();
        }
        f.close();
    }

    QGroupBox* clienteBox = new QGroupBox("Informações do Cliente");
    clienteBox->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            border: 1px solid #404040;
            border-radius: 4px;
            margin-top: 1ex;
            padding: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #4CAF50;
        }
    )");
    QVBoxLayout* clienteLayout = new QVBoxLayout(clienteBox);
    
    clienteLayout->addWidget(new QLabel(QString("Nome: %1").arg(userData["fullName"].toString())));
    clienteLayout->addWidget(new QLabel(QString("Email: %1").arg(userData["email"].toString())));
    clienteLayout->addWidget(new QLabel(QString("Telefone: %1").arg(userData["phone"].toString())));
    clienteLayout->addWidget(new QLabel(QString("NIF: %1").arg(userData["nif"].toString())));
    
    layout->addWidget(clienteBox);

    // Detalhes da encomenda
    QGroupBox* detalhesBox = new QGroupBox("Detalhes da Encomenda");
    detalhesBox->setStyleSheet(clienteBox->styleSheet());
    QVBoxLayout* detalhesLayout = new QVBoxLayout(detalhesBox);
    
    detalhesLayout->addWidget(new QLabel(QString("Data: %1")
        .arg(targetOrder->orderDate.toString("dd/MM/yyyy HH:mm"))));
    
    // Lista de produtos
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; }");
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* itemsLayout = new QVBoxLayout(scrollContent);
    
    for (const OrderItem& item : targetOrder->items) {
        QWidget* itemWidget = new QWidget();
        itemWidget->setStyleSheet("background-color: #363636; border-radius: 4px; padding: 8px; margin: 2px;");
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        
        itemLayout->addWidget(new QLabel(item.productName));
        itemLayout->addWidget(new QLabel(QString("x%1").arg(item.quantity)));
        itemLayout->addWidget(new QLabel(QString("%1€").arg(item.price * item.quantity, 0, 'f', 2)));
        
        itemsLayout->addWidget(itemWidget);
    }
    
    scrollContent->setLayout(itemsLayout);
    scrollArea->setWidget(scrollContent);
    detalhesLayout->addWidget(scrollArea);
    
    // Total
    QLabel* totalLabel = new QLabel(QString("Total: %1€").arg(targetOrder->total, 0, 'f', 2));
    totalLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4CAF50; margin-top: 10px;");
    detalhesLayout->addWidget(totalLabel);
    
    layout->addWidget(detalhesBox);

    // Botão fechar
    QPushButton* closeButton = new QPushButton("Fechar", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px 20px;
            border-radius: 4px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    dialog->exec();
    delete dialog;
}

void MainWindow::finalizarCompra() {
    if (!loggedInUser.isEmpty()) {
        if (carrinho.isEmpty()) {
            QMessageBox::warning(this, "Carrinho Vazio", "Adicione produtos ao carrinho antes de finalizar a compra.");
            return;
        }

        // Criar nova encomenda
        Order order;
        order.orderId = gerarOrderId();
        order.userId = loggedInUser;
        order.orderDate = QDateTime::currentDateTime();
        order.total = 0.0;

        // Adicionar itens do carrinho
        for (auto it = carrinho.begin(); it != carrinho.end(); ++it) {
            const QString& productId = it.key();
            int quantity = it.value();
            ProdutoFull produto = productManager->getProduct(productId);
            
            OrderItem item;
            item.productId = productId;
            item.productName = produto.nome;
            item.quantity = quantity;
            item.price = produto.preco;
            order.items.append(item);
            
            order.total += produto.preco * quantity;

            // Atualizar o stock após confirmar a compra
            productManager->releaseReservation(productId, quantity);
            productManager->commitProductSale(productId, quantity);
        }

        // Salvar a encomenda
        QString error;
        if (salvarEncomenda(order, error)) {
            QMessageBox::information(this, "Compra Finalizada", 
                QString("Encomenda %1 realizada com sucesso!\nTotal: %2€")
                .arg(order.orderId)
                .arg(QString::number(order.total, 'f', 2)));
            
            // Limpar o carrinho após compra bem-sucedida
            limparCarrinho();
        } else {
            QMessageBox::warning(this, "Erro", 
                QString("Não foi possível finalizar a compra: %1").arg(error));
        }
    } else {
        QMessageBox::warning(this, "Login Necessário", 
            "Por favor, faça login para finalizar a compra.");
    }
}