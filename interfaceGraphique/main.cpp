/**
 * \file main.cpp
 * 
 * \author Tristan Brousseau Rigaudie
 * \version 0.1
 * \date 9 decembre 2017
 *
 * Point de départ de l'interface graphique
 *
 */



#include <QApplication>
#include <QtWidgets>
#include <iostream>
#include "maingui.h"

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    MainGui* atriumTracking = new MainGui();
    atriumTracking->show();

    return app.exec();
}
