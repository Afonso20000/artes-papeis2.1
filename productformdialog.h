#ifndef PRODUCTFORMDIALOG_H
#define PRODUCTFORMDIALOG_H

#include <QDialog>
#include "produto.h"

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QLabel;
class QFormLayout;
class QVBoxLayout;

class ProductFormDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductFormDialog(QWidget* parent = nullptr);
    explicit ProductFormDialog(const ProdutoFull& produto, QWidget* parent = nullptr);
    
    ProdutoFull getProduct() const;

private:
    void setupUi();
    void setupConnections();

    QLineEdit* nomeEdit;
    QLineEdit* idEdit;
    QSpinBox* quantidadeEdit;
    QDoubleSpinBox* precoEdit;
    QLineEdit* categoriaEdit;
    QPushButton* imgBtn;
    QPushButton* colorBtn;
    QLabel* imgPathLabel;
    QLabel* colorPreview;
    
    QString savedImagePath;
    QColor chosenColor;
    
    QVBoxLayout* mainLayout;
    QFormLayout* formLayout;
};

#endif // PRODUCTFORMDIALOG_H