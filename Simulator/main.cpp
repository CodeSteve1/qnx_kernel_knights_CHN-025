#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    
    // Optional: Force a dark theme style palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(18, 18, 18));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    qApp->setPalette(darkPalette);
    
    MainWindow w;
    w.show();
    
    return a.exec();
}