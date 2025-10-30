#include "productformdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QColorDialog>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>

ProductFormDialog::ProductFormDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    setupConnections();
    
    // Valores padrão para novo produto
    chosenColor = QColor("#BDB3A3");
    colorPreview->setStyleSheet(
        QString("background-color: %1; border: 1px solid #999;").arg(chosenColor.name())
    );
    quantidadeEdit->setValue(1);
    precoEdit->setValue(1.0);
    categoriaEdit->setText("Geral");
}

ProductFormDialog::ProductFormDialog(const ProdutoFull& produto, QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    setupConnections();
    
    // Preencher com valores existentes
    nomeEdit->setText(produto.nome);
    idEdit->setText(produto.id);
    quantidadeEdit->setValue(produto.quantidade);
    precoEdit->setValue(produto.preco);
    categoriaEdit->setText(produto.categoria);
    savedImagePath = produto.imagePath;
    imgPathLabel->setText(QFileInfo(produto.imagePath).fileName());
    chosenColor = produto.cor;
    colorPreview->setStyleSheet(
        QString("background-color: %1; border: 1px solid #999;").arg(chosenColor.name())
    );
}

void ProductFormDialog::setupUi()
{
    setWindowTitle(tr("Produto"));
    resize(400, 300);
    
    mainLayout = new QVBoxLayout(this);
    
    // Criar campos
    nomeEdit = new QLineEdit(this);
    idEdit = new QLineEdit(this);
    quantidadeEdit = new QSpinBox(this);
    precoEdit = new QDoubleSpinBox(this);
    categoriaEdit = new QLineEdit(this);
    imgBtn = new QPushButton(tr("Escolher imagem..."), this);
    colorBtn = new QPushButton(tr("Escolher cor..."), this);
    imgPathLabel = new QLabel(this);
    colorPreview = new QLabel(this);
    
    // Configurar campos
    quantidadeEdit->setRange(0, 100000);
    precoEdit->setRange(0, 100000);
    precoEdit->setDecimals(2);
    imgPathLabel->setWordWrap(true);
    colorPreview->setFixedSize(40, 20);
    
    // Layout do formulário
    formLayout = new QFormLayout;
    formLayout->addRow(tr("Nome:"), nomeEdit);
    formLayout->addRow(tr("ID:"), idEdit);
    formLayout->addRow(tr("Quantidade:"), quantidadeEdit);
    formLayout->addRow(tr("Preço:"), precoEdit);
    formLayout->addRow(tr("Categoria:"), categoriaEdit);
    formLayout->addRow(tr("Imagem:"), imgBtn);
    formLayout->addRow("", imgPathLabel);
    formLayout->addRow(tr("Cor:"), colorBtn);
    formLayout->addRow("", colorPreview);
    
    mainLayout->addLayout(formLayout);
    
    // Botões OK/Cancelar
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QPushButton* okButton = new QPushButton(tr("OK"), this);
    QPushButton* cancelButton = new QPushButton(tr("Cancelar"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Aplicar tema escuro
    setStyleSheet(R"(
        QDialog { background-color: #2b2b2b; color: #eaeaea; }
        QLabel { color: #eaeaea; }
        QLineEdit, QSpinBox, QDoubleSpinBox { 
            background-color: #4a4a4a; 
            color: #eaeaea; 
            padding: 5px;
            border: 1px solid #666;
            border-radius: 4px;
        }
        QPushButton { 
            background-color: #5a5a5a; 
            color: #ffffff; 
            padding: 6px 12px; 
            border-radius: 4px; 
        }
        QPushButton:hover { background-color: #707070; }
    )");
}

void ProductFormDialog::setupConnections()
{
    connect(imgBtn, &QPushButton::clicked, this, [this]() {
        QString imageFile = QFileDialog::getOpenFileName(this, tr("Escolher imagem"),
            QDir::homePath(), tr("Images (*.png *.jpg *.jpeg *.bmp)"));
        if (!imageFile.isEmpty()) {
            QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir dataDir(dataPath + "/artes-papeis");
            if (!dataDir.exists()) dataDir.mkpath(".");
            QDir imagesDir(dataDir.filePath("images"));
            if (!imagesDir.exists()) imagesDir.mkpath(".");
            QFileInfo fi(imageFile);
            QString target = imagesDir.filePath(fi.fileName());
            QFile::copy(imageFile, target);
            savedImagePath = target;
            imgPathLabel->setText(fi.fileName());
        }
    });

    connect(colorBtn, &QPushButton::clicked, this, [this]() {
        QColor newColor = QColorDialog::getColor(chosenColor, this, tr("Escolher cor do card"));
        if (newColor.isValid()) {
            chosenColor = newColor;
            colorPreview->setStyleSheet(
                QString("background-color: %1; border: 1px solid #999;").arg(chosenColor.name())
            );
        }
    });
}

ProdutoFull ProductFormDialog::getProduct() const
{
    return ProdutoFull{
        nomeEdit->text(),
        precoEdit->value(),
        chosenColor,
        idEdit->text(),
        quantidadeEdit->value(),
        savedImagePath,
        categoriaEdit->text()
    };
}