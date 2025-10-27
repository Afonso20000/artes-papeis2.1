#include <QApplication>
#include <QMainWindow>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.setWindowTitle("Loja de Artesanato");
    window.resize(1024, 768);
    window.show();
    
    return app.exec();
}