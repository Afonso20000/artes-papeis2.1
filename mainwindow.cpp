#include "mainwindow.h"
#include "productcard.h"
#include "productmanager.h"
#include "productformdialog.h"
#include "githubdarktheme.h"
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
#include <QScrollBar>
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
#include <QShortcut>
#include <QKeySequence>
#include <QFileDialog>
#include <QTextStream>
#include <QApplication>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

// Implementação do event filter para ClickableOrderWidget
bool ClickableOrderWidget::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        MainWindow* mainWin = qobject_cast<MainWindow*>(parent());
        if (mainWin) {
            mainWin->mostrarDetalhesEncomenda(m_orderId);
        }
        return true;
    }
    return QObject::eventFilter(obj, event);
}

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

    QLabel* header = new QLabel("🎨");
    header->setStyleSheet(QString("background-color: %1; color: %2; padding: 10px; font-size: 13px; font-weight: 500; border-bottom: 1px solid %3;")
        .arg(GitHubDark::BG_SECONDARY)
        .arg(GitHubDark::TEXT_SECONDARY)
        .arg(GitHubDark::BORDER_DEFAULT));
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QHBoxLayout* navLayout = new QHBoxLayout();
    searchBar = new QLineEdit();
    searchBar->setPlaceholderText("🔍 Buscar produtos...");
    searchBar->setFixedWidth(280);
    searchBar->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            padding: 8px 12px;
            border-radius: 6px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: %4;
            background-color: %5;
        }
    )").arg(GitHubDark::INPUT_BG)
       .arg(GitHubDark::TEXT_PRIMARY)
       .arg(GitHubDark::INPUT_BORDER)
       .arg(GitHubDark::INPUT_FOCUS_BORDER)
       .arg(GitHubDark::BG_PRIMARY));
    connect(searchBar, &QLineEdit::textChanged, this, [this](const QString& text) {
        filtrarProdutos(text);
        
        // Mostrar preview se não estiver na loja
        if (paginas && paginas->currentWidget() != lojaPage) {
            mostrarPreviewPesquisa(text);
        } else {
            esconderPreviewPesquisa();
        }
    });

    // Atalhos: Ctrl+F (pesquisar), / (focar pesquisa), Esc (limpar)
    QShortcut* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        if (searchBar) { searchBar->setFocus(); searchBar->selectAll(); }
    });
    QShortcut* slashShortcut = new QShortcut(QKeySequence(Qt::Key_Slash), this);
    connect(slashShortcut, &QShortcut::activated, this, [this]() {
        if (searchBar) { searchBar->setFocus(); }
    });
    QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (searchBar && !searchBar->text().isEmpty()) { searchBar->clear(); filtrarProdutos(""); }
    });

    QLabel* logo = new QLabel("<b>🎨 Artes & Papéis</b>");
    logo->setStyleSheet(QString("font-size: 20px; color: %1; font-weight: 600;")
        .arg(GitHubDark::TEXT_LINK));
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedWidth(200); // Largura fixa para não mover

    carrinhoIconLabel = new ClickableLabel(this, "🛒 Carrinho (0)");
    carrinhoIconLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 500; padding: 8px 12px; border-radius: 6px;")
        .arg(GitHubDark::TEXT_LINK));
    carrinhoIconLabel->setCursor(Qt::PointingHandCursor);
    connect(carrinhoIconLabel, &ClickableLabel::clicked, this, &MainWindow::mostrarCarrinho);

    // Criar botão de login
    loginButton = new QPushButton("Log In", this);
    loginButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: 1px solid %2;
            padding: 6px 16px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: %3;
            border-color: %3;
        }
        QPushButton:pressed {
            background-color: %4;
        }
    )").arg(GitHubDark::BUTTON_PRIMARY_BG)
       .arg(GitHubDark::BUTTON_PRIMARY_BG)
       .arg(GitHubDark::BUTTON_PRIMARY_HOVER)
       .arg(GitHubDark::ACCENT_PRIMARY));
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::abrirLogin);

    // Layout esquerdo com search bar
    navLayout->addWidget(searchBar, 1);
    
    // Layout central com logo (sempre centralizado)
    QHBoxLayout* centerLayout = new QHBoxLayout();
    centerLayout->addStretch(1);
    centerLayout->addWidget(logo, 0, Qt::AlignCenter);
    centerLayout->addStretch(1);
    navLayout->addLayout(centerLayout, 2);
    
    // Layout direito com carrinho e login
    QHBoxLayout* rightLayout = new QHBoxLayout();
    rightLayout->addWidget(carrinhoIconLabel, 0);
    rightLayout->addSpacing(8);
    
    // Add user name label (initially hidden)
    userNameLabel = new QLabel(this);
    userNameLabel->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 14px;
            font-weight: 500;
            padding: 6px 12px;
            border-radius: 6px;
            background-color: transparent;
        }
        QLabel:hover {
            background-color: %2;
            color: %3;
        }
    )").arg(GitHubDark::TEXT_PRIMARY)
       .arg(GitHubDark::BG_TERTIARY)
       .arg(GitHubDark::TEXT_LINK));
    userNameLabel->hide();
    userNameLabel->setCursor(Qt::PointingHandCursor);
    userNameLabel->installEventFilter(this);
    rightLayout->addWidget(userNameLabel, 0);
    
    rightLayout->addWidget(loginButton, 0);
    navLayout->addLayout(rightLayout, 1);

    mainLayout->addLayout(navLayout);
    
    // Widget de preview de pesquisa (inicialmente escondido)
    searchPreviewWidget = new QWidget(this);
    searchPreviewWidget->setStyleSheet(QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
    )").arg(GitHubDark::BG_SECONDARY)
       .arg(GitHubDark::BORDER_DEFAULT));
    searchPreviewWidget->hide();
    searchPreviewWidget->setMaximumHeight(400);
    mainLayout->addWidget(searchPreviewWidget);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    paginas = new QStackedWidget(this);

    // --------- Página LOJA -----------
    lojaPage = new QWidget;
    QVBoxLayout* lojaLayout = new QVBoxLayout(lojaPage);
    QLabel* sectionTitle = new QLabel("📦 Produtos");
    sectionTitle->setStyleSheet(QString("font-size: 24px; margin: 24px 0 16px 0; font-weight: 600; color: %1;")
        .arg(GitHubDark::TEXT_PRIMARY));
    sectionTitle->setAlignment(Qt::AlignLeft);

    // Botão de editar produtos (aparece apenas para admin)
    editProductsButton = new QPushButton("⚙️ Gerir Produtos");
    editProductsButton->setVisible(false);
    editProductsButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            border: 1px solid %2;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(GitHubDark::ACCENT_RED)
       .arg(GitHubDark::ACCENT_RED)
       .arg("#c5352d"));
    connect(editProductsButton, &QPushButton::clicked, this, &MainWindow::showProductManager);

    // header horizontal para título + edit button
    QHBoxLayout* titleRow = new QHBoxLayout();
    titleRow->addWidget(sectionTitle, 0, Qt::AlignLeft);
    titleRow->addStretch(1);
    titleRow->addWidget(editProductsButton, 0, Qt::AlignRight);
    lojaLayout->addLayout(titleRow);

    // Botões de filtro por categoria
    categoryButtonsWidget = new QWidget();
    QHBoxLayout* categoryLayout = new QHBoxLayout(categoryButtonsWidget);
    categoryLayout->setContentsMargins(0, 10, 0, 16);
    categoryLayout->setSpacing(8);
    
    QStringList categories = {"Todos", "Batismo", "Casamento", "Aniversário", "Natal", "Geral"};
    for (const QString& cat : categories) {
        QPushButton* catBtn = new QPushButton(cat);
        catBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                padding: 8px 16px;
                border-radius: 6px;
                font-size: 14px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: %4;
                border-color: %5;
                color: %6;
            }
            QPushButton:checked {
                background-color: %7;
                border-color: %7;
                color: white;
                font-weight: 600;
            }
        )").arg(GitHubDark::BUTTON_SECONDARY_BG)
           .arg(GitHubDark::TEXT_PRIMARY)
           .arg(GitHubDark::BORDER_DEFAULT)
           .arg(GitHubDark::BUTTON_SECONDARY_HOVER)
           .arg(GitHubDark::INPUT_FOCUS_BORDER)
           .arg(GitHubDark::TEXT_LINK)
           .arg(GitHubDark::ACCENT_BLUE));
        catBtn->setCheckable(true);
        if (cat == "Todos") catBtn->setChecked(true);
        
        connect(catBtn, &QPushButton::clicked, this, [this, cat, categoryLayout]() {
            // Desmarcar todos os outros botões
            for (int i = 0; i < categoryLayout->count(); ++i) {
                if (QPushButton* btn = qobject_cast<QPushButton*>(categoryLayout->itemAt(i)->widget())) {
                    btn->setChecked(false);
                }
            }
            // Marcar apenas o botão clicado
            if (QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender())) {
                clickedBtn->setChecked(true);
            }
            filtrarPorCategoria(cat);
        });
        
        categoryLayout->addWidget(catBtn);
    }
    categoryLayout->addStretch();
    lojaLayout->addWidget(categoryButtonsWidget);

    // Configurar o ProductManager para gerir nossos produtos (instância já criada no inicializador)
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
    QLabel* contatoContent = new QLabel("Email: artespapeis@gmail.com\nTelemóvel: 928052266");
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
    setStyleSheet(GitHubDark::getGlobalStyleSheet());
    resize(1280, 820);

    QHBoxLayout* menuLayout = new QHBoxLayout();
    QStringList labels = {"Início", "Loja", "Carrinho", "Sobre", "Contato"};
    menuButtons.clear(); // Limpar a lista de botões
    for (int i = 0; i < labels.size(); ++i) {
        if (labels[i] == "Carrinho" && isAdmin) {
            continue; // Pular o botão do carrinho para admins
        }
        QPushButton* btn = new QPushButton(labels[i]);
        menuButtons.append(btn); // Armazenar referência ao botão
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background: transparent;
                border: none;
                color: %1;
                font-size: 15px;
                font-weight: 500;
                padding: 8px 16px;
                border-radius: 6px;
            }
            QPushButton:hover {
                background-color: %2;
                color: %3;
            }
            QPushButton:pressed {
                background-color: %4;
            }
        )").arg(GitHubDark::TEXT_PRIMARY)
           .arg(GitHubDark::BG_TERTIARY)
           .arg(GitHubDark::TEXT_LINK)
           .arg(GitHubDark::BUTTON_SECONDARY_HOVER));
        
        // Determinar o índice da página com base no texto do botão
        int pageIndex;
        if (btn->text() == "Início") pageIndex = 0;
        else if (btn->text() == "Loja") pageIndex = 1;
        else if (btn->text() == "Carrinho") pageIndex = 2;
        else if (btn->text() == "Sobre") pageIndex = 3;
        else if (btn->text() == "Contato") pageIndex = 4;
        else pageIndex = 0;

        connect(btn, &QPushButton::clicked, this, [this, pageIndex, btn]() {
            paginas->setCurrentIndex(pageIndex);
            if (btn->text() == "Carrinho") {
                atualizarCarrinhoPagina();
            }
        });
        menuLayout->addWidget(btn);
    }
    mainLayout->insertLayout(3, menuLayout);

    paginas->setCurrentIndex(0);
}

void MainWindow::abrirSobre()   { paginas->setCurrentWidget(sobrePage); esconderPreviewPesquisa();      }
void MainWindow::abrirLoja()    { paginas->setCurrentWidget(lojaPage); esconderPreviewPesquisa();       }
void MainWindow::abrirInicio()  { paginas->setCurrentWidget(inicioPage); esconderPreviewPesquisa();     }
void MainWindow::abrirContato() { paginas->setCurrentWidget(contatoPage); esconderPreviewPesquisa();    }

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
    // atualizar tooltip preview sempre que muda
    mostrarPreviewCarrinho();
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
        vazioLbl->setStyleSheet("color: #8b949e; font-size: 16px; font-weight: 500; padding: 28px; background: #161b22; border: 1px solid #30363d; border-radius: 8px;");
        contentLayout->addWidget(vazioLbl);
    } else {
        double total = 0;
        
        // Header
        QWidget* header = new QWidget(content);
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
    header->setStyleSheet("background-color: #161b22; border: 1px solid #30363d; border-radius: 8px; padding: 10px; color: #c9d1d9;");
        
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
            itemWidget->setStyleSheet("background-color: #161b22; border-radius: 8px; margin: 6px 0; padding: 12px; border: 1px solid #30363d;");

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
            nomeLabel->setStyleSheet("font-weight: 600; color: #c9d1d9; font-size: 14px;");
            
            produtoInfoLayout->addWidget(imgLabel);
            produtoInfoLayout->addWidget(nomeLabel);
            produtoInfoLayout->addStretch();

            // Preço unitário
            QLabel* precoLabel = new QLabel(QString::number(produto.preco, 'f', 2) + "€");
            precoLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
            
            // Controles de quantidade
            QWidget* quantWidget = new QWidget(itemWidget);
            QHBoxLayout* quantLayout = new QHBoxLayout(quantWidget);
            quantWidget->setStyleSheet("border: none;");
            
            QPushButton* minusBtn = new QPushButton("-", quantWidget);
            QPushButton* plusBtn = new QPushButton("+", quantWidget);
            QLabel* quantLabel = new QLabel(QString::number(quantidade), quantWidget);
            
            minusBtn->setFixedSize(26, 26);
            plusBtn->setFixedSize(26, 26);
            QString btnStyle = "QPushButton { background-color: #21262d; color: #c9d1d9; border: 1px solid #30363d; border-radius: 13px; font-size: 16px; font-weight: 600; } "
                               "QPushButton:hover { background-color: #30363d; } "
                               "QPushButton:pressed { background-color: #1f6feb; border-color: #1f6feb; color: #ffffff; }";
            minusBtn->setStyleSheet(btnStyle);
            plusBtn->setStyleSheet(btnStyle);
            quantLabel->setStyleSheet("color: #c9d1d9; font-size: 15px; font-weight: 600;");
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
            totalLabel->setStyleSheet("color: #238636; font-weight: 600; font-size: 14px;");
            
            // Botão remover
            QPushButton* removerBtn = new QPushButton("🗑️", itemWidget);
            removerBtn->setStyleSheet("QPushButton { background: none; border: none; color: #da3633; font-size: 16px; } "
                                    "QPushButton:hover { color: #f85149; }");
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
    totalWidget->setStyleSheet("background-color: #161b22; border-radius: 8px; padding: 20px; margin-top: 20px; border: 1px solid #30363d;");
        
        QLabel* totalLabel = new QLabel("Total:", totalWidget);
    totalLabel->setStyleSheet("color: #8b949e; font-size: 15px; font-weight: 500;");
        QLabel* valorLabel = new QLabel(QString("%1€").arg(QString::number(total, 'f', 2)), totalWidget);
    valorLabel->setStyleSheet("font-size: 20px; color: #238636; font-weight: 600;");
        
        totalLayout->addWidget(totalLabel);
        totalLayout->addStretch();
        totalLayout->addWidget(valorLabel);
        
        contentLayout->addWidget(totalWidget);

        // Adicionar botão "Finalizar Compra"
        QPushButton* finalizarBtn = new QPushButton("Finalizar Compra", content);
        finalizarBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #238636;
                color: #ffffff;
                border: 1px solid #238636;
                padding: 12px 24px;
                border-radius: 6px;
                font-size: 15px;
                font-weight: 600;
                margin-top: 20px;
            }
            QPushButton:hover {
                background-color: #2ea043;
                border-color: #2ea043;
            }
            QPushButton:pressed {
                background-color: #238636;
                border-color: #2ea043;
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
        carrinhoIconLabel->setVisible(loggedInUser.isEmpty() || !isAdmin);
    }

    // Atualizar visibilidade dos botões do menu
    for (QPushButton* btn : menuButtons) {
        if (btn->text() == "Carrinho") {
            btn->setVisible(!isAdmin);
        }
        // Ocultar abas desnecessárias para admin
        if (btn->text() == "Sobre" || btn->text() == "Contato" || btn->text() == "Início") {
            btn->setVisible(!isAdmin); // Mostrar quando NÃO é admin, esconder quando É admin
        }
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
            adminMenuButton = new QPushButton("Admin");
            adminMenuButton->setStyleSheet(R"(
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
            connect(adminMenuButton, &QPushButton::clicked, this, [this]() {
                if (adminPage) {
                    paginas->setCurrentWidget(adminPage);
                }
            });
            menuLayout->addWidget(adminMenuButton);
            
            // Atualizar badge de notificações
            atualizarBadgeAdmin();
        }
        
        // Forçar atualização da página administrativa
        if (paginas->currentWidget() != adminPage) {
            paginas->setCurrentWidget(adminPage);
        }
    } else {
        // Se não for admin, remover o botão Admin se existir
        if (adminMenuButton) {
            adminMenuButton->setVisible(false);
            adminMenuButton->deleteLater();
            adminMenuButton = nullptr;
        }
        
        // Voltar para a página da loja
        if (lojaPage) {
            paginas->setCurrentWidget(lojaPage);
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
    dialog->resize(900, 650);
    dialog->setStyleSheet("QDialog { background-color: #0d1117; color: #c9d1d9; }");

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Título
    QLabel* titleLabel = new QLabel("📦 Minhas Encomendas");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #58a6ff; margin-bottom: 10px;");
    layout->addWidget(titleLabel);

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
        QLabel* emptyLabel = new QLabel("📭 Você ainda não tem encomendas.");
        emptyLabel->setStyleSheet("color: #8b949e; font-size: 18px; padding: 40px; background: #161b22; border-radius: 8px; border: 1px solid #30363d;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    } else {
        QScrollArea* scrollArea = new QScrollArea(dialog);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QWidget* contentWidget = new QWidget(scrollArea);
        QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setSpacing(12);

        // Ordenar encomendas por data (mais recente primeiro)
        std::sort(userOrders.begin(), userOrders.end(), 
                 [](const Order& a, const Order& b) { return a.orderDate > b.orderDate; });

        for (const Order& order : userOrders) {
            // Container clickable para cada encomenda
            QWidget* orderWidget = new QWidget(contentWidget);
            orderWidget->setCursor(Qt::PointingHandCursor);
            orderWidget->setStyleSheet(R"(
                QWidget {
                    background-color: #161b22;
                    border-radius: 8px;
                    border: 1px solid #30363d;
                    padding: 0px;
                }
                QWidget:hover {
                    background-color: #1c2128;
                    border-color: #58a6ff;
                }
            )");
            
            QHBoxLayout* orderLayout = new QHBoxLayout(orderWidget);
            orderLayout->setContentsMargins(16, 16, 16, 16);
            orderLayout->setSpacing(16);

            // Coluna esquerda: Info principal
            QVBoxLayout* leftColumn = new QVBoxLayout();
            leftColumn->setSpacing(8);
            
            // ID e Data
            QHBoxLayout* headerRow = new QHBoxLayout();
            QLabel* orderIdLabel = new QLabel(QString("🧾 <b>%1</b>").arg(order.orderId));
            orderIdLabel->setStyleSheet("font-size: 15px; color: #58a6ff;");
            
            QLabel* dateLabel = new QLabel(order.orderDate.toString("dd/MM/yyyy HH:mm"));
            dateLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
            
            headerRow->addWidget(orderIdLabel);
            headerRow->addStretch();
            headerRow->addWidget(dateLabel);
            leftColumn->addLayout(headerRow);

            // Resumo dos itens
            QString itemsSummary;
            int totalItems = 0;
            for (const OrderItem& item : order.items) {
                totalItems += item.quantity;
            }
            if (order.items.size() == 1) {
                itemsSummary = QString("%1x %2").arg(order.items[0].quantity).arg(order.items[0].productName);
            } else {
                itemsSummary = QString("%1 produtos (%2 itens)").arg(order.items.size()).arg(totalItems);
            }
            
            QLabel* itemsLabel = new QLabel(itemsSummary);
            itemsLabel->setStyleSheet("color: #c9d1d9; font-size: 14px;");
            leftColumn->addWidget(itemsLabel);
            
            // Total
            QLabel* totalLabel = new QLabel(QString("Total: <b>%1€</b>")
                .arg(order.total, 0, 'f', 2));
            totalLabel->setStyleSheet("font-size: 16px; color: #3fb950; margin-top: 4px;");
            leftColumn->addWidget(totalLabel);
            
            orderLayout->addLayout(leftColumn, 3);

            // Coluna direita: Status badge
            QVBoxLayout* rightColumn = new QVBoxLayout();
            rightColumn->setAlignment(Qt::AlignCenter);
            
            QString currentStatus = order.status.isEmpty() ? "pending" : order.status;
            QString statusText = obterTextoEstadoEncomenda(currentStatus);
            QColor statusColor = obterCorEstadoEncomenda(currentStatus);
            
            // Badge de status
            QLabel* statusBadge = new QLabel(statusText);
            statusBadge->setAlignment(Qt::AlignCenter);
            statusBadge->setStyleSheet(QString(R"(
                background-color: %1;
                color: %2;
                padding: 8px 16px;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 600;
            )").arg(statusColor.name())
               .arg(statusColor.lightness() > 128 ? "#000000" : "#ffffff"));
            
            rightColumn->addWidget(statusBadge);
            
            // Indicador de mensagens não lidas (se houver)
            if (!order.chat.isEmpty()) {
                QLabel* chatIndicator = new QLabel(QString("💬 %1 mensagem%2")
                    .arg(order.chat.size())
                    .arg(order.chat.size() > 1 ? "ns" : ""));
                chatIndicator->setStyleSheet("color: #8b949e; font-size: 12px; margin-top: 8px;");
                chatIndicator->setAlignment(Qt::AlignCenter);
                rightColumn->addWidget(chatIndicator);
            }
            
            orderLayout->addLayout(rightColumn, 1);
            
            // Seta indicadora
            QLabel* arrowLabel = new QLabel("›");
            arrowLabel->setStyleSheet("color: #8b949e; font-size: 24px; font-weight: bold;");
            orderLayout->addWidget(arrowLabel, 0, Qt::AlignCenter);

            // Tornar o widget clicável
            orderWidget->installEventFilter(new ClickableOrderWidget(this, order.orderId));

            contentLayout->addWidget(orderWidget);
        }

        contentLayout->addStretch();
        scrollArea->setWidget(contentWidget);
        layout->addWidget(scrollArea);
    }

    // Botão fechar
    QPushButton* closeButton = new QPushButton("Fechar", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #21262d;
            color: #c9d1d9;
            border: 1px solid #30363d;
            padding: 10px 24px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #30363d;
            border-color: #58a6ff;
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

bool MainWindow::atualizarEncomendas(const QVector<Order>& orders, QString& outError) {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath + "/artes-papeis");
    if (!dataDir.exists() && !dataDir.mkpath(".")) {
        outError = "Não foi possível criar diretório de dados.";
        return false;
    }
    QString ordersPath = dataDir.filePath("orders.json");

    QJsonArray ordersArray;
    for (const Order& order : orders) {
        QJsonObject orderObj;
        orderObj["orderId"] = order.orderId;
        orderObj["userId"] = order.userId;
        orderObj["userName"] = order.userName;
        orderObj["orderDate"] = order.orderDate.toString(Qt::ISODate);
        orderObj["status"] = order.status;
        orderObj["lastUpdated"] = order.lastUpdated.toString(Qt::ISODate);
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

        QJsonArray chatArray;
        for (const ChatMessage& msg : order.chat) {
            QJsonObject msgObj;
            msgObj["userId"] = msg.userId;
            msgObj["userName"] = msg.userName;
            msgObj["message"] = msg.message;
            msgObj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
            chatArray.append(msgObj);
        }
        orderObj["chat"] = chatArray;

        ordersArray.append(orderObj);
    }

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
        order.status = obj["status"].toString();
        if (order.status.isEmpty()) order.status = "pending";
        
        QString lastUpdatedStr = obj["lastUpdated"].toString();
        order.lastUpdated = lastUpdatedStr.isEmpty() ? 
                           order.orderDate : 
                           QDateTime::fromString(lastUpdatedStr, Qt::ISODate);

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

        QJsonArray chatArray = obj["chat"].toArray();
        for (const QJsonValue& chatVal : chatArray) {
            QJsonObject msgObj = chatVal.toObject();
            ChatMessage msg;
            msg.userId = msgObj["userId"].toString();
            msg.userName = msgObj["userName"].toString();
            msg.message = msgObj["message"].toString();
            msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
            order.chat.append(msg);
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

double MainWindow::calcularVendasTotais() {
    double total = 0;
    auto orders = carregarEncomendas();
    for (const auto& order : orders) {
        // Contar vendas aceites, processadas, enviadas e entregues
        if (order.status == "accepted" || order.status == "processing" || 
            order.status == "shipped" || order.status == "delivered") {
            total += order.total;
        }
    }
    return total;
}

QMap<QString, int> MainWindow::obterProdutosMaisVendidos(int limite) {
    QMap<QString, int> vendasPorProduto;
    auto orders = carregarEncomendas();
    
    // Contar vendas por produto
    for (const auto& order : orders) {
        // Contar vendas confirmadas (accepted ou estados posteriores)
        if (order.status == "accepted" || order.status == "processing" || 
            order.status == "shipped" || order.status == "delivered") {
            for (const auto& item : order.items) {
                vendasPorProduto[item.productName] += item.quantity;
            }
        }
    }
    
    // Converter para lista para ordenar
    QList<QPair<QString, int>> lista;
    for (auto it = vendasPorProduto.begin(); it != vendasPorProduto.end(); ++it) {
        lista.append({it.key(), it.value()});
    }
    
    // Ordenar por quantidade vendida
    std::sort(lista.begin(), lista.end(), 
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });
    
    // Retornar apenas os top N produtos
    QMap<QString, int> resultado;
    for (int i = 0; i < qMin(limite, lista.size()); ++i) {
        resultado[lista[i].first] = lista[i].second;
    }
    
    return resultado;
}

QMap<QString, int> MainWindow::obterClientesMaisAtivos(int limite) {
    QMap<QString, int> comprasPorCliente;
    auto orders = carregarEncomendas();
    
    // Contar compras por cliente
    for (const auto& order : orders) {
        // Contar compras confirmadas
        if (order.status == "accepted" || order.status == "processing" || 
            order.status == "shipped" || order.status == "delivered") {
            comprasPorCliente[order.userName]++;
        }
    }
    
    // Converter para lista para ordenar
    QList<QPair<QString, int>> lista;
    for (auto it = comprasPorCliente.begin(); it != comprasPorCliente.end(); ++it) {
        lista.append({it.key(), it.value()});
    }
    
    // Ordenar por número de compras
    std::sort(lista.begin(), lista.end(), 
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });
    
    // Retornar apenas os top N clientes
    QMap<QString, int> resultado;
    for (int i = 0; i < qMin(limite, lista.size()); ++i) {
        resultado[lista[i].first] = lista[i].second;
    }
    
    return resultado;
}

QMap<QDate, double> MainWindow::obterVendasPorPeriodo(int dias) {
    QMap<QDate, double> vendasPorDia;
    auto orders = carregarEncomendas();
    
    QDate hoje = QDate::currentDate();
    QDate inicio = hoje.addDays(-dias + 1);
    
    // Inicializar todas as datas com 0
    for (int i = 0; i < dias; ++i) {
        vendasPorDia[inicio.addDays(i)] = 0;
    }
    
    // Somar vendas por dia
    for (const auto& order : orders) {
        if (order.status == "accepted") {
            QDate data = order.orderDate.date();
            if (data >= inicio && data <= hoje) {
                vendasPorDia[data] += order.total;
            }
        }
    }
    
    return vendasPorDia;
}

void MainWindow::setupAdminPage() {
    if (!adminPage) {
        adminPage = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(adminPage);
        
        // Header com título e botão de atualizar
        QHBoxLayout* headerLayout = new QHBoxLayout();
        
        // Título da área administrativa
        QLabel* lbl = new QLabel("Painel de Gestão", adminPage);
        lbl->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50; margin-bottom: 20px;");
        headerLayout->addWidget(lbl);
        
        // Botão de atualizar
        QPushButton* refreshButton = new QPushButton("🔄 Atualizar", adminPage);
        refreshButton->setStyleSheet(R"(
            QPushButton {
                background-color: #4a4a4a;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: #5a5a5a;
            }
        )");
        connect(refreshButton, &QPushButton::clicked, this, [this](){
            atualizarDashboard();
            atualizarEstoque();
            atualizarRelatorios();
            atualizarListaEncomendas();
        });
        headerLayout->addWidget(refreshButton);
        
        layout->addLayout(headerLayout);

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

        // Adicionar Dashboard
        setupDashboardTab();

        // Adicionar aba de Estoque
        setupInventoryTab();

        // Adicionar aba de Relatórios
        setupReportsTab();

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

void MainWindow::setupDashboardTab() {
    QWidget* dashboardTab = new QWidget();
    QVBoxLayout* dashLayout = new QVBoxLayout(dashboardTab);
    
    // Scroll area para todo o conteúdo do dashboard
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(10, 10, 10, 10);

    // Botão de atualizar
    QPushButton* refreshButton = new QPushButton("🔄");
    refreshButton->setFixedSize(32, 32);
    refreshButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4a4a4a;
            color: white;
            border: none;
            border-radius: 16px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #5a5a5a;
        }
        QPushButton:pressed {
            background-color: #404040;
        }
    )");
    refreshButton->setToolTip("Atualizar Dashboard");
    connect(refreshButton, &QPushButton::clicked, this, [this, refreshButton]() {
        refreshButton->setEnabled(false);
        atualizarDashboard();
        atualizarEstoque();
        atualizarRelatorios();
        atualizarListaEncomendas();
        refreshButton->setEnabled(true);
    });
    contentLayout->addWidget(refreshButton, 0, Qt::AlignRight);

    // Estatísticas Gerais em Cards
    QHBoxLayout* statsLayout = new QHBoxLayout();
    
    auto createCard = [](const QString& id, const QString& title, const QString& value, const QString& icon) {
        QWidget* card = new QWidget();
        card->setObjectName(id + "Card");
        card->setStyleSheet("background-color: #363636; border-radius: 8px; padding: 15px; min-width: 200px;");
        QVBoxLayout* layout = new QVBoxLayout(card);
        
        QLabel* iconLabel = new QLabel(icon);
        iconLabel->setStyleSheet("font-size: 24px;");
        QLabel* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("color: #9e9e9e; font-size: 14px;");
        QLabel* valueLabel = new QLabel(value);
        valueLabel->setObjectName(id + "Value");
        valueLabel->setStyleSheet("color: #4CAF50; font-size: 24px; font-weight: bold;");
        
        layout->addWidget(iconLabel);
        layout->addWidget(titleLabel);
        layout->addWidget(valueLabel);
        
        return card;
    };

    // Cards de estatísticas
    QWidget* vendasTotaisCard = createCard("vendasTotais", 
        "Vendas Totais", 
        QString("%1€").arg(calcularVendasTotais(), 0, 'f', 2), 
        "💶");
    
    QWidget* vendasHojeCard = createCard("vendasHoje", 
        "Vendas Hoje", 
        "0€", 
            "📦");

    QWidget* estoqueBaixoCard = createCard("estoqueBaixo", 
        "Estoque Baixo", 
        QString::number(productManager->getLowStockProducts().size()), 
        "⚠️");
    estoqueBaixoCard->findChild<QLabel*>("estoqueBaixoValue")->setStyleSheet(
        "color: #f44336; font-size: 24px; font-weight: bold;");

    QWidget* pedidosPendentesCard = createCard("pedidosPendentes", 
        "Pedidos Pendentes", 
        "0", 
        "📋");
    
    statsLayout->addWidget(vendasTotaisCard);
    statsLayout->addWidget(vendasHojeCard);
    statsLayout->addWidget(estoqueBaixoCard);
    statsLayout->addWidget(pedidosPendentesCard);
    contentLayout->addLayout(statsLayout);

    // Lista de Produtos Mais Vendidos
    QGroupBox* topProdutosGroup = new QGroupBox("Produtos Mais Vendidos");
    topProdutosGroup->setStyleSheet(R"(
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
    
    QVBoxLayout* topProdutosLayout = new QVBoxLayout(topProdutosGroup);
    auto produtosMaisVendidos = obterProdutosMaisVendidos();
    for (auto it = produtosMaisVendidos.begin(); it != produtosMaisVendidos.end(); ++it) {
        QLabel* prodLabel = new QLabel(QString("%1 - %2 unidades").arg(it.key()).arg(it.value()));
        prodLabel->setStyleSheet("color: #ffffff; padding: 5px;");
        topProdutosLayout->addWidget(prodLabel);
    }
    
    // Lista de Clientes Mais Ativos
    QGroupBox* topClientesGroup = new QGroupBox("Clientes Mais Ativos");
    topClientesGroup->setStyleSheet(topProdutosGroup->styleSheet());
    
    QVBoxLayout* topClientesLayout = new QVBoxLayout(topClientesGroup);
    auto clientesMaisAtivos = obterClientesMaisAtivos();
    for (auto it = clientesMaisAtivos.begin(); it != clientesMaisAtivos.end(); ++it) {
        QLabel* clienteLabel = new QLabel(QString("%1 - %2 compras").arg(it.key()).arg(it.value()));
        clienteLabel->setStyleSheet("color: #ffffff; padding: 5px;");
        topClientesLayout->addWidget(clienteLabel);
    }
    
    QHBoxLayout* listsLayout = new QHBoxLayout();
    listsLayout->addWidget(topProdutosGroup);
    listsLayout->addWidget(topClientesGroup);
    contentLayout->addLayout(listsLayout);

    // Gráfico simples de vendas dos últimos 7 dias
    QGroupBox* chartGroup = new QGroupBox("Vendas dos Últimos 7 Dias");
    chartGroup->setStyleSheet(R"(
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
    QVBoxLayout* chartLayout = new QVBoxLayout(chartGroup);
    
    auto vendasPeriodo = obterVendasPorPeriodo(7);
    QLineSeries* series = new QLineSeries();
    int dayIndex = 0;
    for (auto it = vendasPeriodo.begin(); it != vendasPeriodo.end(); ++it) {
        series->append(dayIndex, it.value());
        dayIndex++;
    }
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("");
    chart->legend()->hide();
    chart->setBackgroundBrush(QBrush(QColor("#363636")));
    chart->setTitleBrush(QBrush(Qt::white));
    
    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Dia");
    axisX->setLabelFormat("%d");
    axisX->setTickCount(qMax(2, vendasPeriodo.size()));
    axisX->setLabelsColor(Qt::white);
    axisX->setTitleBrush(QBrush(Qt::white));
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Total €");
    axisY->setLabelFormat("%.2f");
    axisY->setLabelsColor(Qt::white);
    axisY->setTitleBrush(QBrush(Qt::white));
    
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    chartView->setStyleSheet("background: #363636; border-radius: 4px;");
    
    chartLayout->addWidget(chartView);
    contentLayout->addWidget(chartGroup);
    
    // Novo Layout: Gráficos lado a lado
    QHBoxLayout* chartsRowLayout = new QHBoxLayout();
    
    // Gráfico de Barras: Top 5 Produtos Mais Vendidos
    QGroupBox* barChartGroup = new QGroupBox("Top 5 Produtos");
    barChartGroup->setStyleSheet(R"(
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
    QVBoxLayout* barChartLayout = new QVBoxLayout(barChartGroup);
    
    QBarSeries* barSeries = new QBarSeries();
    QBarSet* barSet = new QBarSet("Unidades");
    barSet->setColor(QColor("#58a6ff"));
    
    QStringList productNames;
    auto topProducts = obterProdutosMaisVendidos(5);
    for (auto it = topProducts.begin(); it != topProducts.end(); ++it) {
        *barSet << it.value();
        productNames << it.key().left(15); // Limitar tamanho do nome
    }
    
    barSeries->append(barSet);
    
    QChart* barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("");
    barChart->legend()->setVisible(false);
    barChart->setBackgroundBrush(QBrush(QColor("#363636")));
    barChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QBarCategoryAxis* barAxisX = new QBarCategoryAxis();
    barAxisX->append(productNames);
    barAxisX->setLabelsColor(Qt::white);
    barAxisX->setLabelsAngle(-45);
    
    QValueAxis* barAxisY = new QValueAxis();
    barAxisY->setLabelsColor(Qt::white);
    barAxisY->setLabelFormat("%d");
    
    barChart->addAxis(barAxisX, Qt::AlignBottom);
    barChart->addAxis(barAxisY, Qt::AlignLeft);
    barSeries->attachAxis(barAxisX);
    barSeries->attachAxis(barAxisY);
    
    QChartView* barChartView = new QChartView(barChart);
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->setMinimumHeight(250);
    barChartView->setStyleSheet("background: #363636; border-radius: 4px;");
    
    barChartLayout->addWidget(barChartView);
    chartsRowLayout->addWidget(barChartGroup);
    
    // Gráfico de Pizza: Status das Encomendas
    QGroupBox* pieChartGroup = new QGroupBox("Status das Encomendas");
    pieChartGroup->setStyleSheet(barChartGroup->styleSheet());
    QVBoxLayout* pieChartLayout = new QVBoxLayout(pieChartGroup);
    
    QPieSeries* pieSeries = new QPieSeries();
    
    // Contar encomendas por status
    QVector<Order> allOrders = carregarEncomendas();
    QMap<QString, int> statusCount;
    for (const Order& order : allOrders) {
        statusCount[order.status]++;
    }
    
    // Cores para cada status
    QMap<QString, QColor> statusColors = {
        {"pending", QColor("#e3b341")},
        {"accepted", QColor("#58a6ff")},
        {"processing", QColor("#6e5494")},
        {"shipped", QColor("#0969da")},
        {"delivered", QColor("#3fb950")},
        {"rejected", QColor("#f85149")},
        {"cancelled", QColor("#6e7681")}
    };
    
    for (auto it = statusCount.begin(); it != statusCount.end(); ++it) {
        QString statusLabel = obterTextoEstadoEncomenda(it.key());
        QPieSlice* slice = pieSeries->append(statusLabel, it.value());
        slice->setColor(statusColors.value(it.key(), QColor("#6e7681")));
        slice->setLabelVisible(true);
        slice->setLabelColor(Qt::white);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        
        // Destacar ao passar o mouse
        connect(slice, &QPieSlice::hovered, [slice](bool hovered) {
            slice->setExploded(hovered);
            if (hovered) {
                slice->setLabelFont(QFont("Arial", 10, QFont::Bold));
            } else {
                slice->setLabelFont(QFont("Arial", 9));
            }
        });
    }
    
    QChart* pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->setTitle("");
    pieChart->legend()->setAlignment(Qt::AlignRight);
    pieChart->legend()->setLabelColor(Qt::white);
    pieChart->setBackgroundBrush(QBrush(QColor("#363636")));
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView* pieChartView = new QChartView(pieChart);
    pieChartView->setRenderHint(QPainter::Antialiasing);
    pieChartView->setMinimumHeight(250);
    pieChartView->setStyleSheet("background: #363636; border-radius: 4px;");
    
    pieChartLayout->addWidget(pieChartView);
    chartsRowLayout->addWidget(pieChartGroup);
    
    contentLayout->addLayout(chartsRowLayout);
    
    // Finalizar scroll area
    scrollContent->setLayout(contentLayout);
    scrollArea->setWidget(scrollContent);
    dashLayout->addWidget(scrollArea);
    
    adminTabWidget->addTab(dashboardTab, "Dashboard");
}

void MainWindow::setupInventoryTab() {
    QWidget* inventoryTab = new QWidget();
    QVBoxLayout* invLayout = new QVBoxLayout(inventoryTab);
    
    // Barra de pesquisa
    QLineEdit* searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Pesquisar produtos...");
    searchBox->setStyleSheet(R"(
        QLineEdit {
            background-color: #363636;
            color: #ffffff;
            padding: 8px;
            border: 1px solid #404040;
            border-radius: 4px;
        }
    )");
    invLayout->addWidget(searchBox);
    
    // Tabela de produtos
    QTableWidget* productTable = new QTableWidget();
    productTable->setColumnCount(6);
    productTable->setHorizontalHeaderLabels({"ID", "Nome", "Preço", "Estoque", "Vendidos", "Status"});
    productTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #363636;
            color: #ffffff;
            gridline-color: #404040;
        }
        QHeaderView::section {
            background-color: #4a4a4a;
            color: #ffffff;
            padding: 5px;
            border: none;
        }
        QTableWidget::item {
            padding: 5px;
        }
    )");
    
    // Preencher tabela com produtos
    auto produtos = productManager->getAllProducts();
    productTable->setRowCount(produtos.size());
    for (int i = 0; i < produtos.size(); ++i) {
        const auto& p = produtos[i];
        productTable->setItem(i, 0, new QTableWidgetItem(p.id));
        productTable->setItem(i, 1, new QTableWidgetItem(p.nome));
        productTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.preco, 'f', 2) + "€"));
        productTable->setItem(i, 3, new QTableWidgetItem(QString::number(p.quantidade)));
        
        int vendidos = 0; // TODO: Implementar contagem de vendas por produto
        productTable->setItem(i, 4, new QTableWidgetItem(QString::number(vendidos)));
        
        QString status = p.quantidade <= p.lowThreshold ? "Baixo" : "Normal";
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(status == "Baixo" ? QColor("#f44336") : QColor("#4CAF50"));
        productTable->setItem(i, 5, statusItem);
    }
    
    productTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    invLayout->addWidget(productTable);
    
    adminTabWidget->addTab(inventoryTab, "Estoque");
}

void MainWindow::setupReportsTab() {
    QWidget* reportsTab = new QWidget();
    QVBoxLayout* reportsLayout = new QVBoxLayout(reportsTab);
    
    // Seletor de período
    QComboBox* periodoCombo = new QComboBox();
    periodoCombo->addItems({"Últimos 7 dias", "Últimos 30 dias", "Este mês", "Este ano"});
    periodoCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #363636;
            color: #ffffff;
            padding: 8px;
            border: 1px solid #404040;
            border-radius: 4px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #ffffff;
            width: 0;
            height: 0;
            margin-right: 5px;
        }
    )");
    // Linha superior: período + export CSV
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(periodoCombo);
    topRow->addStretch();
    QPushButton* exportBtn = new QPushButton("Exportar CSV");
    exportBtn->setStyleSheet("QPushButton { background-color: #21262d; color: #c9d1d9; border: 1px solid #30363d; padding: 8px 16px; border-radius: 6px; } QPushButton:hover { background-color: #30363d; }");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportarRelatoriosCSV);
    topRow->addWidget(exportBtn);
    reportsLayout->addLayout(topRow);
    
    // Cards de métricas financeiras
    QHBoxLayout* metricsLayout = new QHBoxLayout();
    
    // Função helper para criar cards
    auto createMetricCard = [](const QString& title, const QString& id, const QString& initialValue, const QString& color) {
        QWidget* card = new QWidget();
        card->setObjectName(id);
        card->setStyleSheet(QString("background-color: #363636; border-radius: 8px; padding: 15px; min-width: 200px;"));
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        
        QLabel* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("color: #9e9e9e; font-size: 14px;");
        QLabel* valueLabel = new QLabel(initialValue);
        valueLabel->setObjectName(id + "_value");
        valueLabel->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold;").arg(color));
        
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);
        return card;
    };
    
    // Calcular valores iniciais
    double faturamentoTotal = calcularVendasTotais();
    QVector<Order> todasOrdens = carregarEncomendas();
    int totalPedidos = todasOrdens.size();
    double mediaDiaria = totalPedidos > 0 ? faturamentoTotal / totalPedidos : 0;
    
    metricsLayout->addWidget(createMetricCard("Faturamento", "faturamento", 
        QString("%1€").arg(faturamentoTotal, 0, 'f', 2), "#4CAF50"));
    metricsLayout->addWidget(createMetricCard("Média por Pedido", "media", 
        QString("%1€").arg(mediaDiaria, 0, 'f', 2), "#2196F3"));
    metricsLayout->addWidget(createMetricCard("Total Pedidos", "pedidos", 
        QString::number(totalPedidos), "#FFC107"));
    
    reportsLayout->addLayout(metricsLayout);
    
    // Tabela de transações
    QTableWidget* transactionTable = new QTableWidget();
    transactionTable->setColumnCount(5);
    transactionTable->setHorizontalHeaderLabels({"Data", "Cliente", "Produtos", "Total", "Status"});
    transactionTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #363636;
            color: #ffffff;
            gridline-color: #404040;
        }
        QHeaderView::section {
            background-color: #4a4a4a;
            color: #ffffff;
            padding: 5px;
            border: none;
        }
        QTableWidget::item {
            padding: 5px;
        }
    )");
    
    // Preencher com algumas transações de exemplo
    auto orders = carregarEncomendas();
    transactionTable->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        transactionTable->setItem(i, 0, new QTableWidgetItem(order.orderDate.toString("dd/MM/yyyy")));
        transactionTable->setItem(i, 1, new QTableWidgetItem(order.userName));
        
        QString produtos;
        for (const auto& item : order.items) {
            if (!produtos.isEmpty()) produtos += ", ";
            produtos += QString("%1 (%2x)").arg(item.productName).arg(item.quantity);
        }
        transactionTable->setItem(i, 2, new QTableWidgetItem(produtos));
        
        transactionTable->setItem(i, 3, new QTableWidgetItem(QString::number(order.total, 'f', 2) + "€"));
        
        QString status = obterTextoEstadoEncomenda(order.status);
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(obterCorEstadoEncomenda(order.status));
        transactionTable->setItem(i, 4, statusItem);
    }
    
    transactionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    reportsLayout->addWidget(transactionTable);
    
    adminTabWidget->addTab(reportsTab, "Relatórios");
}

void MainWindow::setupAdminOrdersTab() {
    QWidget* encomendasTab = new QWidget();
    QVBoxLayout* encomendasLayout = new QVBoxLayout(encomendasTab);
    encomendasLayout->setSpacing(15);
    encomendasLayout->setContentsMargins(15, 15, 15, 15);
    
    // Cabeçalho com título e botão de atualizar
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("📦 Gestão de Encomendas");
    titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: 600; color: %1;")
        .arg(GitHubDark::TEXT_PRIMARY));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QPushButton* refreshBtn = new QPushButton("🔄 Atualizar");
    refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            padding: 8px 16px;
            border: 1px solid %1;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg(GitHubDark::ACCENT_PRIMARY)
       .arg(GitHubDark::BUTTON_PRIMARY_HOVER));
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::atualizarListaEncomendas);
    headerLayout->addWidget(refreshBtn);
    
    encomendasLayout->addLayout(headerLayout);
    
    // Barra de pesquisa
    QLineEdit* searchOrdersEdit = new QLineEdit();
    searchOrdersEdit->setPlaceholderText("🔍 Pesquisar por nome do cliente...");
    searchOrdersEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 10px 12px;
            color: %3;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: %4;
        }
    )").arg(GitHubDark::BG_SECONDARY)
       .arg(GitHubDark::BORDER_DEFAULT)
       .arg(GitHubDark::TEXT_PRIMARY)
       .arg(GitHubDark::ACCENT_PRIMARY));
    
    connect(searchOrdersEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        filtrarEncomendas(text);
    });
    
    encomendasLayout->addWidget(searchOrdersEdit);
    
    // Botões de filtro por status
    QHBoxLayout* statusFilterLayout = new QHBoxLayout();
    statusFilterLayout->setSpacing(8);
    
    QStringList statusList = {"Todos", "Pendente", "Aceite", "Processando", "Enviado", "Entregue", "Rejeitado", "Cancelado"};
    QMap<QString, QString> statusMapping = {
        {"Todos", ""},
        {"Pendente", "pending"},
        {"Aceite", "accepted"},
        {"Processando", "processing"},
        {"Enviado", "shipped"},
        {"Entregue", "delivered"},
        {"Rejeitado", "rejected"},
        {"Cancelado", "cancelled"}
    };
    
    for (const QString& statusName : statusList) {
        QPushButton* filterBtn = new QPushButton(statusName);
        filterBtn->setCheckable(true);
        filterBtn->setChecked(statusName == "Todos");
        
        QString btnStyle = QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                padding: 6px 14px;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: %4;
                border-color: %5;
            }
            QPushButton:checked {
                background-color: %5;
                color: white;
                border-color: %5;
            }
        )").arg(GitHubDark::BG_SECONDARY)
           .arg(GitHubDark::TEXT_PRIMARY)
           .arg(GitHubDark::BORDER_DEFAULT)
           .arg(GitHubDark::BG_TERTIARY)
           .arg(GitHubDark::ACCENT_PRIMARY);
        
        filterBtn->setStyleSheet(btnStyle);
        
        connect(filterBtn, &QPushButton::clicked, this, [this, statusName, statusMapping, statusFilterLayout]() {
            currentOrderStatusFilter = statusMapping[statusName];
            
            // Desmarcar todos os outros botões
            for (int i = 0; i < statusFilterLayout->count(); ++i) {
                QLayoutItem* item = statusFilterLayout->itemAt(i);
                if (QPushButton* btn = qobject_cast<QPushButton*>(item->widget())) {
                    btn->setChecked(btn->text() == statusName);
                }
            }
            
            atualizarListaEncomendas();
        });
        
        statusFilterLayout->addWidget(filterBtn);
    }
    
    statusFilterLayout->addStretch();
    encomendasLayout->addLayout(statusFilterLayout);
    
    // Área de scroll para as encomendas
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QString("QScrollArea { border: none; background: %1; }")
        .arg(GitHubDark::BG_PRIMARY));
    
    // Reduzir velocidade do scroll
    scrollArea->verticalScrollBar()->setSingleStep(10); // Reduzir de 20 (padrão) para 10
    scrollArea->verticalScrollBar()->setPageStep(50);   // Reduzir de 100 (padrão) para 50
    
    QWidget* scrollContent = new QWidget();
    adminOrdersList = new QListWidget(scrollContent);
    adminOrdersList->setStyleSheet(QString(R"(
        QListWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            color: %3;
        }
        QListWidget::item {
            padding: 0px;
            margin: 8px;
            border: none;
            background: transparent;
        }
        QListWidget::item:hover {
            background: transparent;
        }
        QListWidget::item:selected {
            background: transparent;
        }
    )").arg(GitHubDark::BG_PRIMARY)
       .arg(GitHubDark::BORDER_DEFAULT)
       .arg(GitHubDark::TEXT_PRIMARY));
    
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->addWidget(adminOrdersList);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    
    scrollArea->setWidget(scrollContent);
    encomendasLayout->addWidget(scrollArea);

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
        // Filtrar por status se não for "Todos"
        if (!currentOrderStatusFilter.isEmpty() && order.status != currentOrderStatusFilter) {
            continue;
        }
        
        // Carregar informações do usuário
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dataPath + "/artes-papeis/users.json");
        QString userName = order.userId;
        QString userEmail = "";
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                QJsonObject userObj = doc.object().value(order.userId).toObject();
                userName = userObj["fullName"].toString();
                userEmail = userObj["email"].toString();
                if (userName.isEmpty()) userName = order.userId;
            }
            f.close();
        }

        // Criar widget personalizado para o card
        QWidget* orderCard = new QWidget();
        orderCard->setStyleSheet(QString(R"(
            QWidget {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 8px;
                padding: 16px;
            }
        )").arg(GitHubDark::BG_SECONDARY)
           .arg(GitHubDark::BORDER_DEFAULT));
        
        QHBoxLayout* cardLayout = new QHBoxLayout(orderCard);
        cardLayout->setSpacing(16);
        
        // Coluna esquerda - Informações principais
        QVBoxLayout* leftColumn = new QVBoxLayout();
        leftColumn->setSpacing(8);
        
        QLabel* orderIdLabel = new QLabel(QString("🔖 <b>%1</b>").arg(order.orderId));
        orderIdLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600;")
            .arg(GitHubDark::TEXT_LINK));
        
        QLabel* dateLabel = new QLabel(QString("📅 %1").arg(order.orderDate.toString("dd/MM/yyyy HH:mm")));
        dateLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(GitHubDark::TEXT_SECONDARY));
        
        QLabel* clientLabel = new QLabel(QString("👤 <b>%1</b>").arg(userName));
        clientLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(GitHubDark::TEXT_PRIMARY));
        
        if (!userEmail.isEmpty()) {
            QLabel* emailLabel = new QLabel(QString("✉️ %1").arg(userEmail));
            emailLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                .arg(GitHubDark::TEXT_MUTED));
            leftColumn->addWidget(emailLabel);
        }
        
        leftColumn->addWidget(orderIdLabel);
        leftColumn->addWidget(dateLabel);
        leftColumn->addWidget(clientLabel);
        
        // Coluna direita - Status e total
        QVBoxLayout* rightColumn = new QVBoxLayout();
        rightColumn->setSpacing(8);
        rightColumn->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // Badge de status
        QString statusText;
        QString statusColor;
        QString statusBg;
        
        if (order.status == "pending") {
            statusText = "⏳ Pendente";
            statusColor = "#000";
            statusBg = GitHubDark::ACCENT_YELLOW;
        } else if (order.status == "accepted") {
            statusText = "✓ Aceite";
            statusColor = "white";
            statusBg = GitHubDark::ACCENT_PRIMARY;
        } else if (order.status == "processing") {
            statusText = "⚙️ Processando";
            statusColor = "white";
            statusBg = "#6e5494";
        } else if (order.status == "shipped") {
            statusText = "🚚 Enviado";
            statusColor = "white";
            statusBg = "#0969da";
        } else if (order.status == "delivered") {
            statusText = "✅ Entregue";
            statusColor = "white";
            statusBg = GitHubDark::ACCENT_PRIMARY;
        } else if (order.status == "rejected") {
            statusText = "❌ Rejeitado";
            statusColor = "white";
            statusBg = GitHubDark::ACCENT_RED;
        } else if (order.status == "cancelled") {
            statusText = "🚫 Cancelado";
            statusColor = "white";
            statusBg = "#6e7681";
        } else {
            statusText = order.status;
            statusColor = "white";
            statusBg = "#6e7681";
        }
        
        QLabel* statusBadge = new QLabel(statusText);
        statusBadge->setStyleSheet(QString(R"(
            background-color: %1;
            color: %2;
            padding: 6px 12px;
            border-radius: 12px;
            font-size: 12px;
            font-weight: 600;
        )").arg(statusBg).arg(statusColor));
        statusBadge->setAlignment(Qt::AlignCenter);
        
        QLabel* totalLabel = new QLabel(QString("<b>%1€</b>").arg(order.total, 0, 'f', 2));
        totalLabel->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 700;")
            .arg(GitHubDark::ACCENT_PRIMARY));
        totalLabel->setAlignment(Qt::AlignRight);
        
        QLabel* itemsCount = new QLabel(QString("%1 item(s)").arg(order.items.size()));
        itemsCount->setStyleSheet(QString("color: %1; font-size: 11px;")
            .arg(GitHubDark::TEXT_MUTED));
        itemsCount->setAlignment(Qt::AlignRight);
        
        rightColumn->addWidget(statusBadge);
        rightColumn->addWidget(totalLabel);
        rightColumn->addWidget(itemsCount);
        
        cardLayout->addLayout(leftColumn, 3);
        cardLayout->addLayout(rightColumn, 1);
        
        // Adicionar à lista
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, order.orderId);
        item->setSizeHint(orderCard->sizeHint() + QSize(0, 20));
        adminOrdersList->addItem(item);
        adminOrdersList->setItemWidget(item, orderCard);
    }
}

void MainWindow::filtrarEncomendas(const QString& searchText) {
    if (!adminOrdersList) return;
    
    QString lowerSearch = searchText.toLower();
    
    for (int i = 0; i < adminOrdersList->count(); ++i) {
        QListWidgetItem* item = adminOrdersList->item(i);
        QWidget* widget = adminOrdersList->itemWidget(item);
        
        if (lowerSearch.isEmpty()) {
            item->setHidden(false);
            continue;
        }
        
        // Obter o ID da encomenda
        QString orderId = item->data(Qt::UserRole).toString();
        
        // Carregar informação do cliente
        QVector<Order> orders = carregarEncomendas();
        bool found = false;
        
        for (const Order& order : orders) {
            if (order.orderId == orderId) {
                QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                QFile f(dataPath + "/artes-papeis/users.json");
                QString userName = order.userId;
                QString userEmail = "";
                
                if (f.open(QIODevice::ReadOnly)) {
                    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
                    if (doc.isObject()) {
                        QJsonObject userObj = doc.object().value(order.userId).toObject();
                        userName = userObj["fullName"].toString();
                        userEmail = userObj["email"].toString();
                        if (userName.isEmpty()) userName = order.userId;
                    }
                    f.close();
                }
                
                // Verificar se o texto de pesquisa está no nome, email ou ID
                if (userName.toLower().contains(lowerSearch) ||
                    userEmail.toLower().contains(lowerSearch) ||
                    orderId.toLower().contains(lowerSearch)) {
                    found = true;
                }
                break;
            }
        }
        
        item->setHidden(!found);
    }
}

void MainWindow::mostrarDetalhesEncomenda(const QString& orderId) {
    // Carregar todas as encomendas
    QVector<Order> orders = carregarEncomendas();
    
    // Encontrar a encomenda específica
    int orderIndex = -1;
    for (int i = 0; i < orders.size(); ++i) {
        if (orders[i].orderId == orderId) {
            orderIndex = i;
            break;
        }
    }
    
    if (orderIndex == -1) {
        QMessageBox::warning(this, "Erro", "Encomenda não encontrada.");
        return;
    }

    Order& targetOrder = orders[orderIndex];
    
    // Verificar se o usuário tem permissão para ver esta encomenda
    if (!isAdmin && targetOrder.userId != loggedInUser) {
        QMessageBox::warning(this, "Acesso Negado", "Você não tem permissão para ver esta encomenda.");
        return;
    }

    // Criar janela de detalhes
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Detalhes da Encomenda");
    dialog->resize(800, 600);
    dialog->setStyleSheet("QDialog { background-color: #0d1117; color: #c9d1d9; }");

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // Header com ID da encomenda
    QLabel* headerLabel = new QLabel(QString("🧾 Encomenda: %1").arg(targetOrder.orderId));
    headerLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #58a6ff; margin-bottom: 10px;");
    layout->addWidget(headerLabel);

    // Scroll area para todo o conteúdo
    QScrollArea* mainScrollArea = new QScrollArea();
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainScrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(16);

    // Se for admin, mostrar informações do cliente
    if (isAdmin) {
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dataPath + "/artes-papeis/users.json");
        QJsonObject userData;
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                userData = doc.object().value(targetOrder.userId).toObject();
            }
            f.close();
        }

        QGroupBox* clienteBox = new QGroupBox("👤 Informações do Cliente");
        clienteBox->setStyleSheet(R"(
            QGroupBox {
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 6px;
                margin-top: 1ex;
                padding: 16px;
                background: #161b22;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
                color: #58a6ff;
                font-weight: 600;
            }
        )");
        QVBoxLayout* clienteLayout = new QVBoxLayout(clienteBox);
        
        clienteLayout->addWidget(new QLabel(QString("Nome: %1").arg(userData["fullName"].toString())));
        clienteLayout->addWidget(new QLabel(QString("Email: %1").arg(userData["email"].toString())));
        clienteLayout->addWidget(new QLabel(QString("Telefone: %1").arg(userData["phone"].toString())));
        clienteLayout->addWidget(new QLabel(QString("NIF: %1").arg(userData["nif"].toString())));
        
        scrollLayout->addWidget(clienteBox);
    }
    
    // Detalhes da encomenda
    QGroupBox* detalhesBox = new QGroupBox("📋 Detalhes da Encomenda");
    QString groupBoxStyle = R"(
        QGroupBox {
            color: #c9d1d9;
            border: 1px solid #30363d;
            border-radius: 6px;
            margin-top: 1ex;
            padding: 16px;
            background: #161b22;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #58a6ff;
            font-weight: 600;
        }
    )";
    detalhesBox->setStyleSheet(groupBoxStyle);
    QVBoxLayout* detalhesLayout = new QVBoxLayout(detalhesBox);
    
    detalhesLayout->addWidget(new QLabel(QString("📅 Data: %1")
        .arg(targetOrder.orderDate.toString("dd/MM/yyyy HH:mm"))));
    
    // Lista de produtos
    QWidget* itemsContainer = new QWidget();
    QVBoxLayout* itemsLayout = new QVBoxLayout(itemsContainer);
    itemsLayout->setSpacing(8);
    itemsLayout->setContentsMargins(0, 8, 0, 8);
    
    for (const OrderItem& item : targetOrder.items) {
        QWidget* itemWidget = new QWidget();
        itemWidget->setStyleSheet("background-color: #21262d; border-radius: 6px; padding: 10px; border: 1px solid #30363d;");
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        
        QLabel* nameLabel = new QLabel(item.productName);
        nameLabel->setStyleSheet("color: #c9d1d9; font-weight: 500;");
        QLabel* qtyLabel = new QLabel(QString("×%1").arg(item.quantity));
        qtyLabel->setStyleSheet("color: #8b949e;");
        QLabel* priceLabel = new QLabel(QString("%1€").arg(item.price * item.quantity, 0, 'f', 2));
        priceLabel->setStyleSheet("color: #3fb950; font-weight: 600;");
        
        itemLayout->addWidget(nameLabel, 3);
        itemLayout->addWidget(qtyLabel, 1);
        itemLayout->addWidget(priceLabel, 1, Qt::AlignRight);
        
        itemsLayout->addWidget(itemWidget);
    }
    itemsContainer->setLayout(itemsLayout);
    detalhesLayout->addWidget(itemsContainer);
    
    // Total
    QLabel* totalLabel = new QLabel(QString("💰 Total: <b>%1€</b>").arg(targetOrder.total, 0, 'f', 2));
    totalLabel->setStyleSheet("font-size: 18px; color: #3fb950; margin-top: 12px; padding: 12px; background: #21262d; border-radius: 6px; border: 1px solid #30363d;");
    detalhesLayout->addWidget(totalLabel);
    
    scrollLayout->addWidget(detalhesBox);

    // Área de Status
    QGroupBox* statusBox = new QGroupBox("📊 Status da Encomenda");
    statusBox->setStyleSheet(groupBoxStyle);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusBox);

    QString currentStatus = targetOrder.status.isEmpty() ? "pending" : targetOrder.status;
    QString statusText = obterTextoEstadoEncomenda(currentStatus);
    QColor statusColor = obterCorEstadoEncomenda(currentStatus);
    
    QLabel* statusLabel = new QLabel(QString("Status: <b>%1</b>").arg(statusText));
    statusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1; padding: 10px; background: %2; border-radius: 6px;")
        .arg(statusColor.lightness() > 128 ? "#000000" : "#ffffff")
        .arg(statusColor.name()));
    statusLayout->addWidget(statusLabel);

    // Dropdown de estados e botões (apenas para admin)
    if (isAdmin) {
        QComboBox* statusCombo = new QComboBox();
        statusCombo->addItem("Pendente", "pending");
        statusCombo->addItem("Aceite", "accepted");
        statusCombo->addItem("Em Processamento", "processing");
        statusCombo->addItem("Enviada", "shipped");
        statusCombo->addItem("Entregue", "delivered");
        statusCombo->addItem("Recusada", "rejected");
        statusCombo->addItem("Cancelada", "cancelled");
        
        // Selecionar o estado atual
        int currentIndex = statusCombo->findData(currentStatus);
        if (currentIndex >= 0) statusCombo->setCurrentIndex(currentIndex);
        
        statusCombo->setStyleSheet(R"(
            QComboBox {
                background-color: #363636;
                color: #ffffff;
                padding: 8px;
                border: 1px solid #404040;
                border-radius: 4px;
                min-width: 200px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox::down-arrow {
                image: none;
                border-left: 5px solid transparent;
                border-right: 5px solid transparent;
                border-top: 5px solid #ffffff;
            }
        )");
        
        QPushButton* updateStatusBtn = new QPushButton("Atualizar Status");
        updateStatusBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #4CAF50;
                color: white;
                padding: 8px 16px;
                border: none;
                border-radius: 4px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
        )");
        
        connect(updateStatusBtn, &QPushButton::clicked, this, [this, &targetOrder, &orders, statusCombo, statusLabel, dialog]() {
            QString newStatus = statusCombo->currentData().toString();
            
            // Se for recusa ou cancelamento, pedir motivo
            if (newStatus == "rejected" || newStatus == "cancelled") {
                QDialog* reasonDialog = new QDialog(dialog);
                reasonDialog->setWindowTitle(newStatus == "rejected" ? "Recusar Encomenda" : "Cancelar Encomenda");
                reasonDialog->setStyleSheet("QDialog { background-color: #2b2b2b; color: #ffffff; }");
                
                QVBoxLayout* reasonLayout = new QVBoxLayout(reasonDialog);
                QLabel* reasonLabel = new QLabel("Por favor, insira o motivo:");
                reasonLabel->setStyleSheet("color: #ffffff; font-size: 14px;");
                
                QTextEdit* reasonEdit = new QTextEdit();
                reasonEdit->setStyleSheet(R"(
                    QTextEdit {
                        background-color: #363636;
                        color: #ffffff;
                        border: 1px solid #404040;
                        border-radius: 4px;
                        padding: 8px;
                        min-height: 100px;
                    }
                )");
                
                QHBoxLayout* btnLayout = new QHBoxLayout();
                QPushButton* confirmBtn = new QPushButton("Confirmar");
                QPushButton* cancelBtn = new QPushButton("Cancelar");
                
                QString btnStyle = R"(
                    QPushButton {
                        padding: 8px 16px;
                        border: none;
                        border-radius: 4px;
                        font-size: 14px;
                        color: white;
                    }
                )";
                
                confirmBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #f44336; }");
                cancelBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #666666; }");
                
                btnLayout->addWidget(confirmBtn);
                btnLayout->addWidget(cancelBtn);
                
                reasonLayout->addWidget(reasonLabel);
                reasonLayout->addWidget(reasonEdit);
                reasonLayout->addLayout(btnLayout);
                
                connect(cancelBtn, &QPushButton::clicked, reasonDialog, &QDialog::reject);
                connect(confirmBtn, &QPushButton::clicked, reasonDialog, [=, &targetOrder, &orders]() {
                    QString reason = reasonEdit->toPlainText().trimmed();
                    if (reason.isEmpty()) {
                        QMessageBox::warning(reasonDialog, "Erro", "Por favor, insira um motivo.");
                        return;
                    }
                    
                    targetOrder.status = newStatus;
                    targetOrder.lastUpdated = QDateTime::currentDateTime();
                    
                    ChatMessage msg;
                    msg.userId = "admin";
                    msg.userName = "Administrador";
                    msg.message = QString("Encomenda %1: %2").arg(obterTextoEstadoEncomenda(newStatus)).arg(reason);
                    msg.timestamp = QDateTime::currentDateTime();
                    targetOrder.chat.append(msg);
                    
                    QString error;
                    if (atualizarEncomendas(orders, error)) {
                        statusLabel->setText(QString("Status atual: %1").arg(obterTextoEstadoEncomenda(newStatus)));
                        statusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
                            .arg(obterCorEstadoEncomenda(newStatus).name()));
                        QMessageBox::information(reasonDialog, "Status Atualizado", "Status atualizado com sucesso.");
                        atualizarDashboard();
                        atualizarEstoque();
                        atualizarRelatorios();
                        atualizarListaEncomendas();
                        atualizarBadgeAdmin();
                        reasonDialog->accept();
                    }
                });
                
                reasonDialog->exec();
                delete reasonDialog;
            } else {
                // Atualização normal de status
                targetOrder.status = newStatus;
                targetOrder.lastUpdated = QDateTime::currentDateTime();
                
                ChatMessage msg;
                msg.userId = "admin";
                msg.userName = "Administrador";
                msg.message = QString("Status alterado para: %1").arg(obterTextoEstadoEncomenda(newStatus));
                msg.timestamp = QDateTime::currentDateTime();
                targetOrder.chat.append(msg);
                
                QString error;
                if (atualizarEncomendas(orders, error)) {
                    statusLabel->setText(QString("Status atual: %1").arg(obterTextoEstadoEncomenda(newStatus)));
                    statusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
                        .arg(obterCorEstadoEncomenda(newStatus).name()));
                    QMessageBox::information(dialog, "Status Atualizado", "Status atualizado com sucesso.");
                    atualizarDashboard();
                    atualizarEstoque();
                    atualizarRelatorios();
                    atualizarListaEncomendas();
                    atualizarBadgeAdmin();
                }
            }
        });
        
        QHBoxLayout* statusControlLayout = new QHBoxLayout();
        statusControlLayout->addWidget(statusCombo);
        statusControlLayout->addWidget(updateStatusBtn);
        statusLayout->addLayout(statusControlLayout);
        
    // Manter botões antigos para compatibilidade mas ocultos
    if (false) {
        QHBoxLayout* statusBtnsLayout = new QHBoxLayout();
        QPushButton* acceptBtn = new QPushButton("Aceitar Encomenda");
        QPushButton* rejectBtn = new QPushButton("Recusar Encomenda");
        
        QString btnStyle = R"(
            QPushButton {
                padding: 8px 16px;
                border: none;
                border-radius: 4px;
                font-size: 14px;
                color: white;
            }
            QPushButton:hover {
                opacity: 0.9;
            }
        )";
        
        acceptBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #4CAF50; }");
        rejectBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #f44336; }");
        
        // Desabilitar botões se já houver decisão
        if (currentStatus != "pending") {
            acceptBtn->setEnabled(false);
            rejectBtn->setEnabled(false);
        }
        
        connect(acceptBtn, &QPushButton::clicked, this, [this, &targetOrder, &orders, orderIndex, statusLabel]() {
            targetOrder.status = "accepted";
            targetOrder.lastUpdated = QDateTime::currentDateTime();
            QString error;
            if (atualizarEncomendas(orders, error)) {
                statusLabel->setText("Status atual: Aceite");
                QMessageBox::information(nullptr, "Status Atualizado", "Encomenda aceite com sucesso!");
                // Atualizar interface administrativa
                atualizarDashboard();
                atualizarEstoque();
                atualizarRelatorios();
                atualizarListaEncomendas();
            }
        });
        
        connect(rejectBtn, &QPushButton::clicked, this, [this, dialog, &targetOrder, &orders, orderIndex, statusLabel]() {
            // Criar diálogo para comentário de recusa
            QDialog* rejectDialog = new QDialog(dialog);
            rejectDialog->setWindowTitle("Recusar Encomenda");
            rejectDialog->setStyleSheet("QDialog { background-color: #2b2b2b; color: #ffffff; }");
            
            QVBoxLayout* rejectLayout = new QVBoxLayout(rejectDialog);
            QLabel* commentLabel = new QLabel("Por favor, insira um comentário explicando o motivo da recusa:");
            commentLabel->setStyleSheet("color: #ffffff; font-size: 14px;");
            
            QTextEdit* commentEdit = new QTextEdit();
            commentEdit->setStyleSheet(R"(
                QTextEdit {
                    background-color: #363636;
                    color: #ffffff;
                    border: 1px solid #404040;
                    border-radius: 4px;
                    padding: 8px;
                    min-height: 100px;
                }
            )");
            
            QHBoxLayout* btnLayout = new QHBoxLayout();
            QPushButton* confirmBtn = new QPushButton("Confirmar");
            QPushButton* cancelBtn = new QPushButton("Cancelar");
            
            QString btnStyle = R"(
                QPushButton {
                    padding: 8px 16px;
                    border: none;
                    border-radius: 4px;
                    font-size: 14px;
                    color: white;
                }
                QPushButton:hover {
                    opacity: 0.9;
                }
            )";
            
            confirmBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #f44336; }");
            cancelBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #666666; }");
            
            btnLayout->addWidget(confirmBtn);
            btnLayout->addWidget(cancelBtn);
            
            rejectLayout->addWidget(commentLabel);
            rejectLayout->addWidget(commentEdit);
            rejectLayout->addLayout(btnLayout);
            
            connect(cancelBtn, &QPushButton::clicked, rejectDialog, &QDialog::reject);
            connect(confirmBtn, &QPushButton::clicked, rejectDialog, [=, &targetOrder, &orders]() {
                QString comment = commentEdit->toPlainText().trimmed();
                if (comment.isEmpty()) {
                    QMessageBox::warning(rejectDialog, "Erro", "Por favor, insira um motivo para a recusa.");
                    return;
                }
                
                // Atualizar status e adicionar comentário
                targetOrder.status = "rejected";
                targetOrder.lastUpdated = QDateTime::currentDateTime();
                
                ChatMessage msg;
                msg.userId = "admin";
                msg.userName = "Administrador";
                msg.message = "Encomenda recusada: " + comment;
                msg.timestamp = QDateTime::currentDateTime();
                targetOrder.chat.append(msg);
                
                QString error;
                if (atualizarEncomendas(orders, error)) {
                    statusLabel->setText("Status atual: Recusada");
                    
                    // Adicionar a mensagem à lista do chat
                    QString formattedMsg = QString("[%1] %2: %3")
                        .arg(msg.timestamp.toString("dd/MM HH:mm"))
                        .arg(msg.userName)
                        .arg(msg.message);
                    // O chatList será atualizado após fechar o diálogo de rejeição
                    QMessageBox::information(rejectDialog, "Status Atualizado", "Encomenda recusada com sucesso.");
                    // Atualizar interface administrativa
                    atualizarDashboard();
                    atualizarEstoque();
                    atualizarRelatorios();
                    atualizarListaEncomendas();
                    rejectDialog->accept();
                } else {
                    QMessageBox::warning(rejectDialog, "Erro", "Não foi possível atualizar o status da encomenda.");
                }
            });
            
            rejectDialog->setModal(true);
            rejectDialog->exec();
            delete rejectDialog;
        });
        
        statusBtnsLayout->addWidget(acceptBtn);
        statusBtnsLayout->addWidget(rejectBtn);
        statusLayout->addLayout(statusBtnsLayout);
    }
    } // Fim do if(false) - código legacy
    
    scrollLayout->addWidget(statusBox);

    // Área de Chat
    QGroupBox* chatBox = new QGroupBox("💬 Chat");
    chatBox->setStyleSheet(groupBoxStyle);
    QVBoxLayout* chatLayout = new QVBoxLayout(chatBox);

    // Lista de mensagens
    QListWidget* chatListWidget = new QListWidget();
    chatListWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #363636;
            border: 1px solid #404040;
            border-radius: 4px;
            color: #ffffff;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #404040;
        }
    )");

    // Preencher mensagens existentes
    for (const ChatMessage& msg : targetOrder.chat) {
        QString formattedMsg = QString("[%1] %2: %3")
            .arg(msg.timestamp.toString("dd/MM HH:mm"))
            .arg(msg.userName)
            .arg(msg.message);
        QListWidgetItem* item = new QListWidgetItem(formattedMsg);
        chatListWidget->addItem(item);
    }

    // Campo de entrada de mensagem
    QWidget* inputWidget = new QWidget();
    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    QLineEdit* messageInput = new QLineEdit();
    messageInput->setStyleSheet(R"(
        QLineEdit {
            background-color: #404040;
            color: #ffffff;
            border: 1px solid #505050;
            border-radius: 4px;
            padding: 8px;
        }
    )");
    QPushButton* sendButton = new QPushButton("Enviar");
    sendButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");

    connect(sendButton, &QPushButton::clicked, this, [=, &targetOrder, &orders]() {
        QString msg = messageInput->text().trimmed();
        if (!msg.isEmpty()) {
            ChatMessage chatMsg;
            chatMsg.userId = loggedInUser;
            chatMsg.userName = isAdmin ? "Admin" : targetOrder.userName;
            chatMsg.message = msg;
            chatMsg.timestamp = QDateTime::currentDateTime();
            
            targetOrder.chat.append(chatMsg);
            targetOrder.lastUpdated = QDateTime::currentDateTime();
            
            QString error;
            if (atualizarEncomendas(orders, error)) {
                QString formattedMsg = QString("[%1] %2: %3")
                    .arg(chatMsg.timestamp.toString("dd/MM HH:mm"))
                    .arg(chatMsg.userName)
                    .arg(chatMsg.message);
                chatListWidget->addItem(formattedMsg);
                messageInput->clear();
            }
        }
    });
    
    // Também enviar quando pressionar Enter
    connect(messageInput, &QLineEdit::returnPressed, sendButton, &QPushButton::click);

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    chatLayout->addWidget(chatListWidget);
    chatLayout->addWidget(inputWidget);

    scrollLayout->addWidget(chatBox);
    
    // Finalizar scroll area
    scrollContent->setLayout(scrollLayout);
    mainScrollArea->setWidget(scrollContent);
    layout->addWidget(mainScrollArea);

    // Botões de ação
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    // Botão fechar
    QPushButton* closeButton = new QPushButton("Fechar", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #21262d;
            color: #c9d1d9;
            border: 1px solid #30363d;
            padding: 10px 24px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #30363d;
            border-color: #58a6ff;
        }
    )");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    actionLayout->addStretch();
    actionLayout->addWidget(closeButton);
    layout->addLayout(actionLayout);

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

        // Obter o nome completo do utilizador
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile userFile(dataPath + "/artes-papeis/users.json");
        if (userFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(userFile.readAll());
            if (doc.isObject()) {
                QJsonObject userObj = doc.object().value(loggedInUser).toObject();
                order.userName = userObj["fullName"].toString();
                if (order.userName.isEmpty()) {
                    order.userName = loggedInUser; // Fallback para username se não houver nome completo
                }
            }
            userFile.close();
        } else {
            order.userName = loggedInUser; // Fallback para username se o arquivo não existir
        }

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
            // som de notificação
            QApplication::beep();
            
            // Limpar o carrinho após compra bem-sucedida
            limparCarrinho();

            // Atualizar interface administrativa se necessário
            if (isAdmin && adminTabWidget) {
                atualizarDashboard();
                atualizarEstoque();
                atualizarRelatorios();
            }
            
            // Atualizar badge de notificações do admin
            atualizarBadgeAdmin();
        } else {
            QMessageBox::warning(this, "Erro", 
                QString("Não foi possível finalizar a compra: %1").arg(error));
        }
    } else {
        QMessageBox::warning(this, "Login Necessário", 
            "Por favor, faça login para finalizar a compra.");
    }
}

void MainWindow::atualizarDashboard() {
    if (!adminTabWidget) return;
    
    // Localizar a aba do Dashboard
    for (int i = 0; i < adminTabWidget->count(); i++) {
        if (adminTabWidget->tabText(i) == "Dashboard") {
            QWidget* dashTab = adminTabWidget->widget(i);
            
            // Calcular métricas
            double vendasTotais = calcularVendasTotais();
            QVector<Order> orders = carregarEncomendas();
            int totalPedidos = orders.size();
            int encomendasPendentes = 0;
            double vendasHoje = 0.0;
            QDate hoje = QDate::currentDate();
            
            for (const Order& order : orders) {
                if (order.status.isEmpty() || order.status == "pending") {
                    encomendasPendentes++;
                }
                // Contar vendas finalizadas (delivered ou accepted)
                if (order.orderDate.date() == hoje && 
                    (order.status == "delivered" || order.status == "accepted")) {
                    vendasHoje += order.total;
                }
            }
            
            // Atualizar número de produtos com estoque baixo
            int produtosBaixoEstoque = productManager->getLowStockProducts().size();
            
            // Atualizar os cards de estatísticas
            auto updateCard = [](QWidget* parent, const QString& title, const QString& value, const QString& icon = "") {
                QWidget* card = parent->findChild<QWidget*>(title + "Card");
                if (card) {
                    QLabel* valueLabel = card->findChild<QLabel*>(title + "Value");
                    if (valueLabel) {
                        valueLabel->setText(value);
                    }
                }
            };
            
            updateCard(dashTab, "vendasTotais", QString("%1€").arg(vendasTotais, 0, 'f', 2), "💶");
                updateCard(dashTab, "vendasHoje", QString("%1€").arg(vendasHoje, 0, 'f', 2), "📦");
            updateCard(dashTab, "estoqueBaixo", QString::number(produtosBaixoEstoque), "⚠️");
            updateCard(dashTab, "pedidosPendentes", QString::number(encomendasPendentes), "📋");
            
            // Atualizar listas de produtos mais vendidos e clientes mais ativos
            auto updateList = [](QGroupBox* group, const QMap<QString, int>& data, const QString& format) {
                if (!group) return;
                QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(group->layout());
                if (!layout) return;

                // Limpar layout existente, mantendo o primeiro widget (título se houver)
                while (layout->count() > 1) {
                    QLayoutItem* item = layout->takeAt(1);
                    if (item->widget()) delete item->widget();
                    delete item;
                }
                
                // Adicionar novos items
                for (auto it = data.begin(); it != data.end(); ++it) {
                    QLabel* label = new QLabel(format.arg(it.key()).arg(it.value()));
                    label->setStyleSheet("color: #ffffff; padding: 5px;");
                    layout->addWidget(label);
                }
                layout->addStretch();
            };

            QGroupBox* produtosGroup = dashTab->findChild<QGroupBox*>("produtosMaisVendidosGroup");
            QGroupBox* clientesGroup = dashTab->findChild<QGroupBox*>("clientesMaisAtivosGroup");

            updateList(produtosGroup, 
                      obterProdutosMaisVendidos(5), 
                      QString("%1 - %2 unidades"));
            updateList(clientesGroup, 
                      obterClientesMaisAtivos(5), 
                      QString("%1 - %2 compras"));

            break;
        }
    }
}

void MainWindow::atualizarEstoque() {
    if (!adminTabWidget) return;
    
    // Localizar a aba de Estoque
    for (int i = 0; i < adminTabWidget->count(); i++) {
        if (adminTabWidget->tabText(i) == "Estoque") {
            QWidget* stockTab = adminTabWidget->widget(i);
            
            // Encontrar a tabela de produtos
            QTableWidget* table = stockTab->findChild<QTableWidget*>();
            if (table) {
                // Limpar tabela
                table->setRowCount(0);
                
                // Preencher com dados atualizados
                auto produtos = productManager->getAllProducts();
                table->setRowCount(produtos.size());
                for (int row = 0; row < produtos.size(); ++row) {
                    const auto& p = produtos[row];
                    table->setItem(row, 0, new QTableWidgetItem(p.id));
                    table->setItem(row, 1, new QTableWidgetItem(p.nome));
                    table->setItem(row, 2, new QTableWidgetItem(QString::number(p.preco, 'f', 2) + "€"));
                    table->setItem(row, 3, new QTableWidgetItem(QString::number(p.quantidade)));
                    
                    int vendidos = 0; // TODO: Implementar contagem de vendas
                    table->setItem(row, 4, new QTableWidgetItem(QString::number(vendidos)));
                    
                    QString status = p.quantidade <= p.lowThreshold ? "Baixo" : "Normal";
                    QTableWidgetItem* statusItem = new QTableWidgetItem(status);
                    statusItem->setForeground(status == "Baixo" ? QColor("#f44336") : QColor("#4CAF50"));
                    table->setItem(row, 5, statusItem);
                }
            }
            break;
        }
    }
}

void MainWindow::atualizarRelatorios() {
    if (!adminTabWidget) return;
    
    // Localizar a aba de Relatórios
    for (int i = 0; i < adminTabWidget->count(); i++) {
        if (adminTabWidget->tabText(i) == "Relatórios") {
            QWidget* reportsTab = adminTabWidget->widget(i);
            
            // Atualizar métricas financeiras
            double faturamentoTotal = calcularVendasTotais();
            QVector<Order> todasOrdens = carregarEncomendas();
            int totalPedidos = todasOrdens.size();
            double mediaDiaria = totalPedidos > 0 ? faturamentoTotal / totalPedidos : 0;

            if (QLabel* faturamentoLabel = reportsTab->findChild<QLabel*>("faturamento_value")) {
                faturamentoLabel->setText(QString("%1€").arg(faturamentoTotal, 0, 'f', 2));
            }
            if (QLabel* mediaLabel = reportsTab->findChild<QLabel*>("media_value")) {
                mediaLabel->setText(QString("%1€").arg(mediaDiaria, 0, 'f', 2));
            }
            if (QLabel* pedidosLabel = reportsTab->findChild<QLabel*>("pedidos_value")) {
                pedidosLabel->setText(QString::number(totalPedidos));
            }
            
            // Atualizar tabela de transações
            QTableWidget* table = reportsTab->findChild<QTableWidget*>();
            if (table) {
                // Limpar tabela
                table->setRowCount(0);
                
                // Carregar transações atualizadas
                auto orders = carregarEncomendas();
                table->setRowCount(orders.size());
                for (int row = 0; row < orders.size(); ++row) {
                    const auto& order = orders[row];
                    table->setItem(row, 0, new QTableWidgetItem(order.orderDate.toString("dd/MM/yyyy")));
                    table->setItem(row, 1, new QTableWidgetItem(order.userName));
                    
                    QString produtos;
                    for (const auto& item : order.items) {
                        if (!produtos.isEmpty()) produtos += ", ";
                        produtos += QString("%1 (%2x)").arg(item.productName).arg(item.quantity);
                    }
                    table->setItem(row, 2, new QTableWidgetItem(produtos));
                    
                    table->setItem(row, 3, new QTableWidgetItem(QString::number(order.total, 'f', 2) + "€"));
                    
                    QString status = obterTextoEstadoEncomenda(order.status);
                    QTableWidgetItem* statusItem = new QTableWidgetItem(status);
                    statusItem->setForeground(obterCorEstadoEncomenda(order.status));
                    table->setItem(row, 4, statusItem);
                }
            }
            break;
        }
    }
}
void MainWindow::filtrarProdutos(const QString& searchText) {
    if (!productsGrid || !productsWidget) return;
    
    QString lowerSearch = searchText.toLower();
    QVector<ProdutoFull> produtos = productManager->getAllProducts();
    
    // Limpar grid atual
    while (productsGrid->count() > 0) {
        QLayoutItem* it = productsGrid->takeAt(0);
        if (!it) break;
        if (QWidget* w = it->widget()) { w->deleteLater(); }
        delete it;
    }
    
    const int cols = 4;
    int idx = 0;
    
    for (const auto &pf : produtos) {
        // Filtrar por categoria primeiro
        if (currentCategory != "Todos" && pf.categoria != currentCategory) {
            continue;
        }
        
        // Depois filtrar por texto de pesquisa
        if (!lowerSearch.isEmpty()) {
            QString produtoNome = pf.nome.toLower();
            QString produtoId = pf.id.toLower();
            if (!produtoNome.contains(lowerSearch) && !produtoId.contains(lowerSearch)) {
                continue;
            }
        }
        
        ProductCard* card = new ProductCard(pf, this);
        card->setAvailableStock(productManager->getAvailableStock(pf.id));
        connect(card, &ProductCard::compraProduto, this, &MainWindow::adicionarAoCarrinho);
        int row = idx / cols;
        int col = idx % cols;
        productsGrid->addWidget(card, row, col, Qt::AlignTop);
        ++idx;
    }
    
    // Se não há resultados, mostrar mensagem
    if (idx == 0) {
        QLabel* noResults = new QLabel("Nenhum produto encontrado");
        noResults->setStyleSheet("color: #9e9e9e; font-size: 18px; padding: 40px;");
        noResults->setAlignment(Qt::AlignCenter);
        productsGrid->addWidget(noResults, 0, 0, 1, cols);
    }
}

void MainWindow::mostrarPreviewPesquisa(const QString& searchText) {
    if (!searchPreviewWidget) return;
    
    // Esconder se texto vazio
    if (searchText.trimmed().isEmpty()) {
        esconderPreviewPesquisa();
        return;
    }
    
    // Limpar conteúdo anterior
    QLayout* oldLayout = searchPreviewWidget->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    QVBoxLayout* previewLayout = new QVBoxLayout(searchPreviewWidget);
    previewLayout->setContentsMargins(12, 12, 12, 12);
    previewLayout->setSpacing(8);
    
    // Título
    QLabel* titleLabel = new QLabel("🔍 Resultados da Pesquisa");
    titleLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600; padding-bottom: 8px;")
        .arg(GitHubDark::TEXT_PRIMARY));
    previewLayout->addWidget(titleLabel);
    
    // Scroll area para os resultados
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(8);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    
    // Buscar produtos que correspondem
    QString lowerSearch = searchText.toLower();
    QVector<ProdutoFull> produtos = productManager->getAllProducts();
    int resultCount = 0;
    const int maxResults = 5; // Limitar a 5 resultados
    
    for (const auto& pf : produtos) {
        if (resultCount >= maxResults) break;
        
        QString produtoNome = pf.nome.toLower();
        QString produtoId = pf.id.toLower();
        
        if (produtoNome.contains(lowerSearch) || produtoId.contains(lowerSearch)) {
            // Criar card de preview para o produto
            QWidget* productPreview = new QWidget();
            productPreview->setStyleSheet(QString(R"(
                QWidget {
                    background-color: %1;
                    border: 1px solid %2;
                    border-radius: 6px;
                    padding: 8px;
                }
                QWidget:hover {
                    border-color: %3;
                    background-color: %4;
                }
            )").arg(GitHubDark::BG_TERTIARY)
               .arg(GitHubDark::BORDER_DEFAULT)
               .arg(GitHubDark::ACCENT_PRIMARY)
               .arg(GitHubDark::BG_OVERLAY));
            productPreview->setCursor(Qt::PointingHandCursor);
            
            QHBoxLayout* previewItemLayout = new QHBoxLayout(productPreview);
            previewItemLayout->setSpacing(12);
            
            // Imagem do produto (pequena)
            QLabel* imgLabel = new QLabel();
            imgLabel->setFixedSize(60, 60);
            if (!pf.imagePath.isEmpty()) {
                QPixmap px(pf.imagePath);
                if (!px.isNull()) {
                    imgLabel->setPixmap(px.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    imgLabel->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
                        .arg(pf.cor.name()));
                    imgLabel->setAlignment(Qt::AlignCenter);
                    imgLabel->setText("📷");
                }
            } else {
                imgLabel->setStyleSheet(QString("background-color: %1; border-radius: 4px; font-size: 24px;")
                    .arg(GitHubDark::BG_OVERLAY));
                imgLabel->setAlignment(Qt::AlignCenter);
                imgLabel->setText("📷");
            }
            previewItemLayout->addWidget(imgLabel);
            
            // Informações do produto
            QVBoxLayout* infoLayout = new QVBoxLayout();
            infoLayout->setSpacing(4);
            
            QLabel* nameLabel = new QLabel(pf.nome);
            nameLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600;")
                .arg(GitHubDark::TEXT_PRIMARY));
            
            QLabel* priceLabel = new QLabel(QString("%1€").arg(pf.preco, 0, 'f', 2));
            priceLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500;")
                .arg(GitHubDark::ACCENT_PRIMARY));
            
            int stock = productManager->getAvailableStock(pf.id);
            QLabel* stockLabel = new QLabel(QString("Stock: %1").arg(stock));
            stockLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                .arg(stock > 0 ? GitHubDark::TEXT_SECONDARY : GitHubDark::ACCENT_RED));
            
            infoLayout->addWidget(nameLabel);
            infoLayout->addWidget(priceLabel);
            infoLayout->addWidget(stockLabel);
            
            previewItemLayout->addLayout(infoLayout, 1);
            
            // Botão de adicionar ao carrinho
            QPushButton* addBtn = new QPushButton("🛒");
            addBtn->setFixedSize(36, 36);
            addBtn->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    color: white;
                    border: none;
                    border-radius: 18px;
                    font-size: 16px;
                }
                QPushButton:hover {
                    background-color: %2;
                }
            )").arg(GitHubDark::ACCENT_PRIMARY)
               .arg(GitHubDark::BUTTON_PRIMARY_HOVER));
            addBtn->setEnabled(stock > 0);
            
            connect(addBtn, &QPushButton::clicked, this, [this, id = pf.id]() {
                adicionarAoCarrinho(id);
                esconderPreviewPesquisa();
                if (searchBar) searchBar->clear();
            });
            
            previewItemLayout->addWidget(addBtn);
            
            // Fazer o card inteiro clicável para ir à loja
            productPreview->installEventFilter(new QObject(productPreview));
            connect(productPreview, &QWidget::destroyed, this, [this, productPreview]() {
                // Cleanup if needed
            });
            
            // Adicionar evento de clique para navegar para a loja
            productPreview->setProperty("productId", pf.id);
            
            scrollLayout->addWidget(productPreview);
            resultCount++;
        }
    }
    
    if (resultCount == 0) {
        QLabel* noResults = new QLabel("Nenhum produto encontrado");
        noResults->setStyleSheet(QString("color: %1; font-size: 13px; padding: 20px;")
            .arg(GitHubDark::TEXT_MUTED));
        noResults->setAlignment(Qt::AlignCenter);
        scrollLayout->addWidget(noResults);
    } else if (resultCount >= maxResults) {
        QLabel* moreLabel = new QLabel(QString("... e mais resultados. Vá para a Loja para ver todos."));
        moreLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-style: italic; padding: 8px;")
            .arg(GitHubDark::TEXT_MUTED));
        moreLabel->setAlignment(Qt::AlignCenter);
        scrollLayout->addWidget(moreLabel);
    }
    
    scrollLayout->addStretch();
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);
    
    previewLayout->addWidget(scrollArea);
    searchPreviewWidget->setLayout(previewLayout);
    searchPreviewWidget->show();
}

void MainWindow::esconderPreviewPesquisa() {
    if (searchPreviewWidget) {
        searchPreviewWidget->hide();
    }
}

void MainWindow::filtrarPorCategoria(const QString& categoria) {
    currentCategory = categoria;
    // Aplicar os filtros atuais (pesquisa + categoria)
    filtrarProdutos(searchBar ? searchBar->text() : QString());
}

void MainWindow::atualizarBadgeAdmin() {
    if (!adminMenuButton) return;
    
    // Contar encomendas pendentes
    QVector<Order> orders = carregarEncomendas();
    int pendingCount = 0;
    for (const Order& order : orders) {
        if (order.status.isEmpty() || order.status == "pending") {
            pendingCount++;
        }
    }
    
    // Atualizar texto do botão com badge
    if (pendingCount > 0) {
        adminMenuButton->setText(QString("Admin (%1)").arg(pendingCount));
    } else {
        adminMenuButton->setText("Admin");
    }
}


QString MainWindow::obterTextoEstadoEncomenda(const QString& status) {
    if (status.isEmpty() || status == "pending") return "Pendente";
    if (status == "accepted") return "Aceite";
    if (status == "processing") return "Em Processamento";
    if (status == "shipped") return "Enviada";
    if (status == "delivered") return "Entregue";
    if (status == "rejected") return "Recusada";
    if (status == "cancelled") return "Cancelada";
    return "Desconhecido";
}

QColor MainWindow::obterCorEstadoEncomenda(const QString& status) {
    if (status.isEmpty() || status == "pending") return QColor("#FFC107"); // Amarelo
    if (status == "accepted") return QColor("#2196F3"); // Azul
    if (status == "processing") return QColor("#9C27B0"); // Roxo
    if (status == "shipped") return QColor("#FF9800"); // Laranja
    if (status == "delivered") return QColor("#4CAF50"); // Verde
    if (status == "rejected") return QColor("#f44336"); // Vermelho
    if (status == "cancelled") return QColor("#757575"); // Cinza
    return QColor("#9e9e9e"); // Cinza claro
}

void MainWindow::mostrarPreviewCarrinho() {
    if (carrinho.isEmpty()) return;
    
    // Criar tooltip personalizado
    QString tooltipText = "<div style='background-color: #161b22; padding: 12px; border-radius: 6px;'>";
    tooltipText += "<p style='font-size: 14px; font-weight: 600; color: #c9d1d9; margin-bottom: 8px;'>🛒 Carrinho</p>";
    
    double total = 0;
    int itemCount = 0;
    for (auto it = carrinho.begin(); it != carrinho.end() && itemCount < 3; ++it, ++itemCount) {
        const QString& id = it.key();
        int quantidade = it.value();
        ProdutoFull produto = productManager->getProduct(id);
        
        tooltipText += QString("<p style='font-size: 12px; color: #8b949e; margin: 4px 0;'>%1x %2 - %3€</p>")
            .arg(quantidade)
            .arg(produto.nome)
            .arg(produto.preco * quantidade, 0, 'f', 2);
        
        total += produto.preco * quantidade;
    }
    
    if (carrinho.size() > 3) {
        tooltipText += QString("<p style='font-size: 12px; color: #6e7681; margin: 4px 0;'>... e mais %1 itens</p>")
            .arg(carrinho.size() - 3);
    }
    
    tooltipText += QString("<p style='font-size: 13px; font-weight: 600; color: #238636; margin-top: 8px; padding-top: 8px; border-top: 1px solid #21262d;'>Total: %1€</p>")
        .arg(total, 0, 'f', 2);
    tooltipText += "</div>";
    
    carrinhoIconLabel->setToolTip(tooltipText);
}

void MainWindow::exportarRelatoriosCSV() {
    if (!adminTabWidget) return;
    // Localizar a aba Relatórios e tabela
    QTableWidget* table = nullptr;
    for (int i = 0; i < adminTabWidget->count(); ++i) {
        if (adminTabWidget->tabText(i) == "Relatórios") {
            table = adminTabWidget->widget(i)->findChild<QTableWidget*>();
            break;
        }
    }
    if (!table) {
        QMessageBox::warning(this, "Exportar CSV", "Tabela de relatórios não encontrada.");
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, "Exportar Relatórios CSV", QDir::homePath()+"/relatorios.csv", "CSV (*.csv)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Exportar CSV", "Não foi possível criar arquivo.");
        return;
    }
    QTextStream out(&file);
    // Cabeçalhos
    QStringList headers;
    for (int c = 0; c < table->columnCount(); ++c) headers << table->horizontalHeaderItem(c)->text();
    out << headers.join(',') << '\n';
    // Linhas
    for (int r = 0; r < table->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < table->columnCount(); ++c) {
            QTableWidgetItem* item = table->item(r, c);
            cols << (item ? item->text().replace('\n',' ').replace(',',';') : "");
        }
        out << cols.join(',') << '\n';
    }
    file.close();
    QMessageBox::information(this, "Exportar CSV", "Relatórios exportados com sucesso.");
}
