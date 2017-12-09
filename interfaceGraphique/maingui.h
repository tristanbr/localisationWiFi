#ifndef MAINGUI_H
#define MAINGUI_H

#include <QtWidgets>
#include <QMainWindow>
#include <QTcpServer>


/** 1 metre en x ou en y equivaut respectivement a SCALE_X ou a SCALE_Y pixels sur la carte*/
#define SCALE_X 66
#define SCALE_Y 56
/** Les pixels qui definissent le point (0,0)*/
#define OFFSET_X 494
#define OFFSET_Y 88





#define SERVER_ADD "10.200.0.9"




class MainGui : public QMainWindow
{
Q_OBJECT
public:
    explicit MainGui(QWidget *parent = 0);
    ~MainGui();


private:
    void setup();
    void setMenu();
    void setUI();
    void startServer();

    QImage * img;
    QLabel* imgDisplay;

    QPushButton* start;
    QPushButton* stop;
    QComboBox* showCombobox;

    QTextEdit* x;
    QTextEdit* y;
    QTimer * timer;
    QElapsedTimer * timeElapsed;
    QPainter * painter;

    float x_pres;
    float y_pres;
    float vx_pres;
    float vy_pres;
    float x_pred;
    float y_pred;
    /**  State est soit -1 si ARRET, 0 si SUIVRE, 1 si LOCALISER*/
    int state;
    bool dotOn;
    bool isConnected;
    bool stateChanged;

    float Dx;
    float Dy;
    QTcpServer * tcpServer;

    QTcpSocket * soc;

signals:

public slots:
	/**
	* \fn void StartTracking()
	* \brief Est appelée lorsque le bouton ACTION est appuyé. Si on est en mode ARRET
	*		le nouveau mode est soit MESURER, LOCALISER ou SUIVRE. La position x et y entrée dans les boites
	*		vont être envoyées à l'appareil.
	*/
    void StartTracking();
	/**
	* \fn void flashImg()
	* \brief Permet de faire clignoter le points de localisation
	*
	*/	
    void flashImg();
	/**
	* \fn void stopTracking
	* \brief Est appelée lorsque le bouton ARRET est appuyé.
	*
	*/	
    void stopTracking();
	/**
	* \fn void rcvData()
	* \brief Est appelée à la reception de données. On va alors mettre a jour la 
	*		position dépendemment du mode.
	*
	*/
	void rcvData();
    /**
	* \fn void newTCPConnection()
	* \brief Est appelée lorsqu'il y a une demande de connexion.
	*
	*/
    void newTCPConnection();
	/**
	* \fn void newTCPConnection()
	* \brief Est appelée lorsqu'il y a une perte de connexion.
	*
	*/
    void tcpDisconnected();
};

#endif // MAINGUI_H
