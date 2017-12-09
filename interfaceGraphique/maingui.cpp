/**
 * \file maingui.cpp
 * 
 * \author Tristan Brousseau Rigaudie
 * \version 0.1
 * \date 9 decembre 2017
 *
 * Interface graphique permettant d'interagir avec l'appareil.
 *
 */

 #if defined (__WIN32__)
	#include <WinSock2.h>
#endif


#include "maingui.h"
#include <QDialog>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QImage>
#include <iostream>
#include <stdexcept>
#include <QTextStream>
#include <QtNetwork>




using namespace std;

MainGui::MainGui(QWidget *parent) : QMainWindow(parent)
{
    img = new QImage("atrium.JPG");
   if(img->isNull()){
        cout << "NO" << endl;
   }else cout << "YES" << endl;


   /** On part au point (0,0)*/
   x_pres = OFFSET_X;
   y_pres = OFFSET_Y;
   dotOn = false;
   isConnected = false;
   state = -1; // ARRET
   Dx =0;
   Dy =0;
   vx_pres =0;
   vy_pres =0;
   stateChanged = false;

   timer = new QTimer(this);
   connect(timer, SIGNAL(timeout()), this, SLOT(flashImg()));

   timeElapsed = new QElapsedTimer();

   painter = new QPainter();

    setup();
}

MainGui::~MainGui()
{
    // Fin de la convo
    if(isConnected == true){
        QTextStream endConvo(soc);
        endConvo << "F" << endl;
    }

    delete img;
    delete start;
    delete stop;
    delete x;
    delete y;
    delete imgDisplay;
    delete timer;
    delete painter;
    delete timeElapsed;
}

void MainGui :: setup(){

    startServer();
    setMenu();
    setUI();
}

void MainGui::startServer(){
    tcpServer = new QTcpServer(this);

    if(tcpServer->listen(QHostAddress(SERVER_ADD), 80) ==false) {
        cout << "LISTEN ERROR" << endl;
    }

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newTCPConnection()));
}

void MainGui:: setMenu(){
    /*Pris du cours INF1010 - Raphael Beamonte*/
    // On crée un bouton 'Exit'
    QAction* exit = new QAction(tr("E&xit"), this);
    // On ajoute un raccourci clavier qui simulera l'appui sur ce bouton (Ctrl+Q)
    exit->setShortcuts(QKeySequence::Quit);
    // On connecte le clic sur ce bouton avec l'action de clore le programme
    connect(exit, SIGNAL(triggered()), this, SLOT(close()));
    // On crée un nouveau menu 'File'
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    // Dans lequel on ajoute notre bouton 'Exit'
    fileMenu->addAction(exit);

}

void MainGui::setUI(){

    //Titre
    QLabel* title = new QLabel("ATRIUM POLYMTL");
    title->setAlignment(Qt::AlignCenter);

    QFont font( "Helvetica", 23, QFont::Bold);
    title->setFont(font);


    QFont fontBouton( "Helvetica", 10);

    //Boite de selection
    showCombobox = new QComboBox(this);
    showCombobox->addItem("Suivre"); // Index 0
    showCombobox->addItem("Localiser"); // Index 1
    showCombobox->addItem("Mesurer"); // Index 2
    //showCombobox->addItem("Calibrer"); // Index 3

    showCombobox->setFont(fontBouton);


    QLabel* espace = new QLabel(" ",this);
    espace->setFont(fontBouton);

    // Position en X
    QLabel* xLabel = new QLabel("Position en X : ",this);
    xLabel->setFont(fontBouton);
    x = new QTextEdit(this);
    x->setFixedHeight(26);
    x->setFixedWidth(120);
    x->setFont(fontBouton);
    x->insertPlainText("0");

    // Position en Y
    QLabel* yLabel = new QLabel("Position en Y : ",this);
    yLabel->setFont(fontBouton);
    y = new QTextEdit();
    y->setFixedHeight(26);
    y->setFixedWidth(120);
    y->setFont(fontBouton);
    y->insertPlainText("0");
    QLabel* metres1 = new QLabel("\n[m]",this);
    metres1->setFont(fontBouton);

    QLabel* metres = new QLabel("\n[m]", this);
    metres->setFont(fontBouton);


    //Boutons

    start = new QPushButton(this);
    QPalette bGreen = start->palette();
    bGreen.setColor(QPalette::Button, QColor(Qt::green));
    start->setText("ACTION");
    start->setAutoFillBackground(true);
    start->setPalette(bGreen);
    start->setFont(fontBouton);
    start->update();

    connect(start, SIGNAL(clicked()),this, SLOT(StartTracking()));

    stop = new QPushButton(this);
    QPalette bblue = stop->palette();
    bblue.setColor(QPalette::Button, QColor(Qt::red));
    stop->setText("ARRÊT");
    stop->setAutoFillBackground(true);
    stop->setPalette(bblue);
    stop->setFont(fontBouton);
    stop->update();

    connect(stop, SIGNAL(clicked()),this, SLOT(stopTracking()));

    QVBoxLayout* XLayout = new QVBoxLayout();
    XLayout->addWidget(xLabel);
    XLayout->addWidget(x);
    QVBoxLayout* YLayout = new QVBoxLayout();
    YLayout->addWidget(yLabel);
    YLayout->addWidget(y);

    QHBoxLayout* boutonLayout = new QHBoxLayout();
    boutonLayout->addWidget(showCombobox);
    boutonLayout->addWidget(espace);
    boutonLayout->addLayout(XLayout);
    boutonLayout->addWidget(metres1);
    boutonLayout->addLayout(YLayout);
    boutonLayout->addWidget(metres);
    boutonLayout->addWidget(start);
    boutonLayout->addWidget(stop);


    QVBoxLayout * mainLayout = new QVBoxLayout();
    imgDisplay = new QLabel(this);
    imgDisplay->setPixmap(QPixmap::fromImage(*img));
    imgDisplay->adjustSize();

    mainLayout->addWidget(title);
    mainLayout->addWidget(imgDisplay);
    mainLayout->addLayout(boutonLayout);


    QWidget* widget = new QWidget(this);
    widget->setLayout(mainLayout);
    setCentralWidget(widget);


    setWindowTitle("ATRIUM Tracking");

}




/**
* \fn void StartTracking()
* \brief Est appelée lorsque le bouton ACTION est appuyé. Si on est en mode ARRET
*		le nouveau mode est soit MESURER, LOCALISER ou SUIVRE. La position x et y entrée dans les boites
*		vont être envoyées à l'appareil.
*/
void MainGui::StartTracking()
{
	/**On regarde si on est en mode ARRET ou non*/
    if(state != -1) return;

    if(timer->isActive() ==  false){

        timer->start(500);
    }


    QString text = x->toPlainText();
    cout << "x :"<<text.toStdString() << endl;

    //Convertir l'entrée X en INT
    int x_pos =-1;
    try {
      x_pos = stoi(text.toStdString());
    }
    catch(...) {
      x_pos = -1;
    }


    text = y->toPlainText();
    cout << "y :"<<text.toStdString() << endl;
    //Convertir l'entrée Y en INT
    int y_pos =-1;
    try {
      y_pos = stoi(text.toStdString());
    }
    catch(...) {
      y_pos = -1;
    }

    if(x_pos >= 0) x_pres = x_pos;
    else x_pres = 0;
    if(y_pos >= 0) y_pres = y_pos;
    else y_pres = 0;

    if(showCombobox->currentIndex() == 1){ // MODE Localiser

        // Convertir la position en pixel
        int pixelX = x_pres*SCALE_X+OFFSET_X;
        int pixelY = y_pres*SCALE_Y+OFFSET_Y;

        // state Suivre
        state = 1;

        img->load("atrium.JPG");


        QPen pen;

        painter->begin(img);

        // On colore le point du centre
        pen.setWidth(20);
        pen.setCapStyle(Qt::RoundCap);
        pen.setColor(QColor(0, 0, 255, 200));
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(pen);
        painter->drawPoint(pixelX,pixelY);

        dotOn = true;
        // On colore un cercle centr/e au point du centre
        pen.setColor(QColor(0, 0, 255, 45));
        pen.setWidth(2);
        painter->setPen(pen);

        painter->setBrush(QColor(0, 0, 255, 45));
        painter->drawEllipse(pixelX - SCALE_X*2,pixelY -SCALE_Y*2,SCALE_X*4,SCALE_Y*4);
        painter->end();
        imgDisplay->setPixmap(QPixmap::fromImage(*img));

        timeElapsed->start();

        // INITIALISATION du filtre Alpha Beta
        if(isConnected == true){
            QTextStream msg(soc);
            msg << "I" << endl;
            msg << (int)x_pres<< ","<< (int)y_pres<<",";
            stateChanged =true;
        }

    }else if(showCombobox->currentIndex() == 0){
        // MODE SUIVRE

        state = 0;
        img->load("atrium.JPG");
        img->save("atriumTrajectory.JPG");
        timeElapsed->start();
        dotOn = false;
        // INITIALISATION du filtre Alpha Beta
        if(isConnected == true){
            QTextStream msg(soc);
            msg << "I" << endl;
            msg << (int)x_pres<< ","<< (int)y_pres<<",";
            stateChanged =true;
        }

    }else if(showCombobox->currentIndex() == 2){
           state = 2;
           dotOn = false;
           timeElapsed->start();
        // INITIALISATION du filtre Alpha Beta
        if(isConnected == true){
            QTextStream msg(soc);
            msg << "M" << endl;
            msg << (int)x_pres<< ","<< (int)y_pres<<",";
            stateChanged = true;
        }

    }




}
/**
* \fn void flashImg()
* \brief Permet de faire clignoter le points de localisation
*
*/

void MainGui::flashImg(){

    if(state == -1) return;
    else if(state > 0){
        cout <<"flashImg::state = 1" << endl;
        img->load("atrium.JPG");
    }
    else if (state == 0){
        img->load("atriumTrajectory.JPG");
    }
    // Convertir la position en pixel
    int pixelX = x_pres*SCALE_X+OFFSET_X;
    int pixelY = y_pres*SCALE_Y+OFFSET_Y;


    QPen pen;

    painter->begin(img);


    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(QColor(0, 0, 255, 200));
    painter->setRenderHint(QPainter::Antialiasing,true);

    if(dotOn == false){
        // On colore le point du centre

        pen.setWidth(20);
        painter->setPen(pen);
        painter->drawPoint(pixelX,pixelY);
        dotOn = true;
    }else{
        dotOn = false;
    }

    // On colore un cercle centr/e au point du centre
    pen.setColor(QColor(0, 0, 255, 45));
    pen.setWidth(2);
    painter->setPen(pen);
    painter->setBrush(QColor(0, 0, 255, 45));
    painter->drawEllipse(pixelX - SCALE_X*2,pixelY -SCALE_Y*2,SCALE_X*4,SCALE_Y*4);
    painter->end();

    imgDisplay->setPixmap(QPixmap::fromImage(*img));

}


/**
* \fn void stopTracking
* \brief Est appelée lorsque le bouton ARRET est appuyé.
*
*/

void MainGui::stopTracking(){

    if(timer->isActive()) timer->stop();
   // if(timerTest->isActive()) timerTest->stop();

    if(state == -1) return;


    QPen pen;

    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(QColor(0, 0, 255, 200));
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setPen(pen);

    if (state ==2){
        QTextStream msg(soc);
        msg << "S" << endl; // On fait une pause sur les mesures.
        stateChanged =true;

        img->load("atrium.JPG");
        painter->begin(img);
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(pen);


    }
    else if(state == 1){
        img->load("atrium.JPG");
        painter->begin(img);
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(pen);


    }else if(state == 0){
        img->load("atriumTrajectory.JPG");
        painter->begin(img);
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(pen);


        if(Dx != 0.0 && Dy != 0.0 ){

           float p0_x = Dx+x_pres;
           float p0_y = Dy+y_pres;



           QPainterPath path;
           path.moveTo(p0_x*SCALE_X+OFFSET_X, p0_y*SCALE_Y+OFFSET_Y);
           path.quadTo(x_pred*SCALE_X+OFFSET_X, y_pred*SCALE_Y+OFFSET_Y,x_pres*SCALE_X+OFFSET_X, y_pres*SCALE_Y+OFFSET_Y);
           painter->drawPath(path);
        }
    }

    int pixelX = x_pres*SCALE_X+OFFSET_X;
    int pixelY = y_pres*SCALE_Y+OFFSET_Y;


    // On colore un cercle centr/e au point du centre
    pen.setColor(QColor(0, 0, 255, 45));
    painter->setPen(pen);
    painter->setBrush(QColor(0, 0, 255, 45));
    painter->drawEllipse(pixelX - SCALE_X*2,pixelY -SCALE_Y*2,SCALE_X*4,SCALE_Y*4);


    // On colore le point du centre
    pen.setColor(QColor(0, 0, 255, 200));
    pen.setWidth(20);
    painter->setPen(pen);
    painter->drawPoint(pixelX,pixelY);

    painter->end();
    imgDisplay->setPixmap(QPixmap::fromImage(*img));
    dotOn = false;
    state = -1;

}


/**
* \fn void rcvData()
* \brief Est appelée à la reception de données. On va alors mettre a jour la 
*		position dépendemment du mode.
*
*/

void MainGui::rcvData(){
    int inc = 0;
    char data[7];
    char c;
    float p_x;
    float p_y;
    QString ligne;
    QTextStream msg(soc);
    // On repond afin de continuer la localisation

    if(stateChanged == false){
        msg << "C" << endl;
    }else stateChanged = false;

    if(state == -1 || (timeElapsed->isValid()&&timeElapsed->elapsed() < 1900)){
        // on vide le msg qu'on a recu.
        while(inc < 4){
            ligne = soc->read(&c,1);
            if(c == 's'){inc ++;}
        }

    }
    else{
        timeElapsed->invalidate();


        // On lit x present
        memset(data, '\0',7);
        inc=0;
        while(1){
            ligne = soc->read(&c,1);
            if(c == 's')break;
            if(inc < 6){data[inc] = c;}
            inc ++;

        }
        p_x =atof(data);
        cout << "p_x" << p_x << endl;
        // On lit x predit
        memset(data, '\0',7);
        inc=0;
        while(1){
            ligne = soc->read(&c,1);
            if(c == 's')break;
            if(inc < 6){data[inc] = c;}
            inc ++;
        }
        x_pred = atof(data);
        cout << "x_pred" << x_pred << endl;
        // On lit y present
        memset(data, '\0',7);
        inc=0;
        while(1){
            ligne = soc->read(&c,1);
            if(c == 's')break;
            if(inc < 6){data[inc] = c;}
            inc ++;
        }
        p_y = atof(data);
        cout << "p_y" << p_y <<endl;
        // On lit y predit
        memset(data, '\0',7);
        inc=0;
        while(1){
            ligne = soc->read(&c,1);
            if(c == 's')break;
            if(inc < 6){data[inc] = c;}
            inc ++;
        }
        y_pred = atof(data);
         cout << "y_pred" << y_pred << endl;


        Dx+= x_pres - p_x;
        Dy+= y_pres - p_y;



        if(Dx*Dx+Dy*Dy > 0.5){
            x_pres = p_x + Dx;
            y_pres = p_y + Dy;


            Dx = 0;
            Dy = 0;
            if(state == 0){
                img->load("atriumTrajectory.JPG");
            }
            else{
                img->load("atrium.JPG");
            }




            QPen pen;
            painter->begin(img);
            pen.setWidth(2);
            pen.setCapStyle(Qt::RoundCap);
            pen.setColor(QColor(0, 0, 255, 200));
            painter->setRenderHint(QPainter::Antialiasing,true);
            painter->setPen(pen);

            if(state == 0){
                //on dessine le chemin
                QPainterPath path;
                path.moveTo(x_pres*SCALE_X+OFFSET_X, y_pres*SCALE_Y+OFFSET_Y);
                path.quadTo(x_pred*SCALE_X+OFFSET_X, y_pred*SCALE_Y+OFFSET_Y,p_x*SCALE_X+OFFSET_X, p_y*SCALE_Y+OFFSET_Y);
                painter->drawPath(path);
                painter->end();

                // on sauvegarde l'image
                img->save("atriumTrajectory.JPG");
            }

            //on dessine la zone de position.

            painter->begin(img);

            pen.setWidth(20);
            painter->setPen(pen);
            painter->drawPoint(p_x*SCALE_X+OFFSET_X, p_y*SCALE_Y+OFFSET_Y);

            dotOn = false;
            pen.setColor(QColor(0, 0, 255, 45));
            pen.setWidth(2);
            painter->setPen(pen);
            painter->setBrush(QColor(0, 0, 255, 45));
            painter->drawEllipse(p_x*SCALE_X+OFFSET_X -SCALE_X*2, p_y*SCALE_Y+OFFSET_Y-SCALE_Y*2, SCALE_X*4, SCALE_Y*4);
            painter->end();
            imgDisplay->setPixmap(QPixmap::fromImage(*img));
        }


        x_pres = p_x;
        y_pres = p_y;
        x->clear();
        y->clear();
        x->insertPlainText(QString::number(x_pres));
        y->insertPlainText(QString::number(y_pres));


    }
}


/**
* \fn void newTCPConnection()
* \brief Est appelée lorsqu'il y a une demande de connexion.
*
*/
void MainGui::newTCPConnection(){

    cout << "Connexion !" << endl;
    soc = tcpServer->nextPendingConnection();
    connect(soc, SIGNAL(readyRead()), this, SLOT(rcvData()));
    connect(soc, SIGNAL(disconnected()), this, SLOT(tcpDisconnected()));
    connect(soc, SIGNAL(disconnected()), soc, SLOT(deleteLater()));
    QTextStream ack(soc);
    ack << "C" << endl;
    isConnected = true;
    tcpServer->pauseAccepting();
}


/**
* \fn void newTCPConnection()
* \brief Est appelée lorsqu'il y a une perte de connexion.
*
*/
void MainGui::tcpDisconnected(){

    isConnected = false;
    tcpServer->resumeAccepting();
}

