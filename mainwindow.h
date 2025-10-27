#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void createMenus();
    
    QWidget *centralWidget;
    QListWidget *productList;
    QPushButton *addButton;
    QPushButton *removeButton;
    QLabel *totalLabel;

private slots:
    void addProduct();
    void removeProduct();
};

#endif