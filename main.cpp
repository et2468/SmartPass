#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"

#include <iostream>
#include <windows.h>

int main(int argc, char* argv[])
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    std::cout << "DEBUG MODE: Console attached successfully!" << std::endl;

    QApplication app(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    MainWindow w;
    w.show();
    return app.exec();

}