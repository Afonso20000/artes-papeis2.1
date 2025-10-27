#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Permite adicionar produtos apenas através do código
    void addProductFromCode(const QString &name, double price);

private:
    void setupUI();
    void createMenus();

    QWidget *centralWidget;
    QListWidget *productList;
    QLabel *totalLabel;
    double total = 0.0;
};

#endif