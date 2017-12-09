/***************************************************
* file:     localisation.cpp
* Author:  Tristan Brousseau-Rigaudi
*/
#include "localisation.h"
#include <math.h>

using namespace std;


/**
    \fn int toMatrice(float* matrice, int nbPointsX, int nbPointsY, int nbAPid);
    \brief  Remplit la matrice avec le fichier mesuresOrdonnees qui est le fichier contenant les RSS de références.
	\return -1 si il y a une erreur d'ouverture de fichier
*/

int toMatrice(float* matrice, int nbPointsX, int nbPointsY, int nbAPid){
    int x=0;
    int y=0;
    int APid=0;

    float moyenne;
    int rss;
    int nb;
    int total;
    char unInt[10];
    char charToRead;
    ifstream inFile;
    int i=1;

    int offset = 0;

    inFile.open("mesuresOrdonnees");
    if(inFile.fail()){
        cout << "Probleme d'ouverture"<<endl;
        return -1;
    }


    nb = 0;
    moyenne = 0;
    total = 0;
    inFile >> charToRead;
    while(!inFile.eof()){

        if(charToRead == '-'){
            unInt[0] = charToRead;

        }else if(charToRead == 'x' || charToRead == 'p' || charToRead == 'A'){
                    inFile.ignore(20, '\n');

        }else if(charToRead > 0x2F && charToRead < 0x3A){
            unInt[i++] = charToRead;
        }else if(charToRead ==','){
           sscanf(unInt, "%d", &rss);
           memset(unInt, 0, sizeof(unInt));
           i = 1;
           moyenne += rss;
           nb+=1;

        }else if(charToRead == ';'){

            sscanf(unInt, "%d", &rss);
            memset(unInt, 0, sizeof(unInt));
            moyenne += rss;
            nb+=1;
            moyenne = (float)moyenne/(float)nb;
            nb = 0;
            i = 1;
            total+=1;
            cout << moyenne <<",";

            *matrice = moyenne;

            offset++;

            if(offset < nbPointsX*nbPointsY*nbAPid){

                matrice++;

            }else{

                break;

            }

            moyenne = 0;

        }
        inFile >> charToRead;
    }

    cout << "nb de points" << total/nbAPid  << endl;
    inFile.close();
    return 0;

}

/**
    \fn void getSquareDistance(const float * matriceRef ,float* matricePos, int nbAP);
    \brief  retourne la distance euclidienne au carre entre matriceRef et matriceRef pour la dimension définie par nbAP.

*/


float getSquareDistance(const float* matriceRef ,float* matricePos, int nbAP){

    float resultat = 0;
    int m = 0;

    for(int i = 0; i < nbAP; i ++){
        if(int(matriceRef[i]) == -80 && int(matricePos[i]) == -80){
            m++;
        }
        else if(int(matriceRef[i]) != -80 && int(matricePos[i]) != -80){
            resultat += ((matriceRef[i] - matricePos[i])*(matriceRef[i] - matricePos[i]));
            m++;
        }
    }

    if(m>0){
        return (resultat/(float)m);
    }
    else
        return 1000.0;

}

/**
    \fn void alphaBetaFilter(float * x_m, float * y_m, float Dt);
    \brief Applique le filtre alpha beta sur *x_m et *y_m. Si Dt == -1 
			le filtre alpha beta est initialsé avec (*x_m, *y_m) comme position initiale
			et (0,0) comme vitesse initiale.

*/

void alphaBetaFilter(float * x_m, float * y_m, float Dt){
    float beta = 0.018858641;
    float alpha = 0.3;
    //float beta = 1;
    //float alpha =1;
    static float x_pred = 0;
    static float y_pred= 0;
    static float x = 0;
    static float y = 0;
    static float v_x = 0;
    static float v_y= 0;

    if((int)Dt == -1){
        // INITIALISATION de la posistion
		int i = 0;
		x = *x_m;
		y = *y_m;
		x_pred = *x_m;
		y_pred = *y_m;
		v_x = 0;
		v_y = 0;
        return;
    }


	x = x_pred + alpha*(x_m[0] - x_pred);
	v_x = v_x + beta*(x_m[0] - x_pred)/Dt;

	y = y_pred + alpha*(y_m[0] - y_pred);
	v_y = v_y + beta*(y_m[0] - y_pred)/Dt;

	x_pred = x + v_x*Dt;
	y_pred = y + v_y*Dt;


    // On retourne "la meilleur estimation" et l'estimation de prediction

    x_m[0] = x;
    x_m[1] = x_pred;
    y_m[0] = y;
    y_m[1] = y_pred;

}


/**
    \fn void changeFreq(int freq)
    \brief Changer la frequence du mode monitore.
*/

void changeFreq(int freq){
        char command[32];
        sprintf(command, "sudo iw dev mon7 set freq %d", freq);
        system(command);
}


/**
    \fn int connectToServer(int port, const char* server_add)
    \brief Se connecter au serveur
    \return socket's file descriptor
*/
int connectToServer(int port, const char* server_add){

    struct sockaddr_in remote_addr;
    int fd;

    /* Zeroing remote_addr struct */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Construct remote_addr struct */
    remote_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_add, &(remote_addr.sin_addr));
    remote_addr.sin_port = htons(port);

    /* Create client socket */
     printf("creating socket\n");
     fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( fd == -1)
    {
            fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

            return -1;
    }

    // Set socket to go throught the wlan interface
    char opt[7] = "wlan0\0";
    setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, opt, 5);

    printf("I just created socket now i will connect");

    /* Connect to the server */
    if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
    {
            fprintf(stderr, "Error on connect --> %s\n", strerror(errno));
            close(fd);
           return -1;
    }


    return fd;

}

/**
    \fn int setPostition(float *x, float *y, int fd);
    \brief Reçoit les positions du serveur et les transme en float
    \return -1 si il y a une erreur
*/


int setPostition(float *x, float *y, int fd){

	char charac;
	int len =0;
	int tampon = 0;
	int rc;

	while(1){
		rc = recv(fd,&charac,1,0);
		if(rc < 0){
			printf("error recv ptSize : %d \n", len);
			close(fd);
			return -1;
		}
		if(charac == ',')break;
		tampon = tampon*10;
		tampon = tampon + charac - '0';

		len += rc;
	}
	x[0] = (float)tampon;
	printf("temp %d\n", tampon);


	tampon = 0;
	len = 0;
	while(1){
		rc = recv(fd,&charac,1,0);
		if(rc < 0){
			printf("error recv ptSize : %d \n", len);
			close(fd);
			return -1;
		}
		if(charac == ',')break;
		tampon = tampon*10;
		tampon = tampon + charac - '0';

		len += rc;
	}



	 y[0] = (float)tampon;
	 printf("INIT alphaBeta %.2f, %.2f", *x, *y);
	//INITIALISATION du filtre alpha Beta
	alphaBetaFilter(x, y, -1);
	return 0;

}


