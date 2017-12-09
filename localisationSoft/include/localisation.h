/***************************************************
* file:     localisation.h
* Author:  Tristan Brousseau-Rigaudie
*/

#ifndef LOCALISATION_H
#define LOCALISATION_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>




/**
    \fn int toMatrice(float* matrice, int nbPointsX, int nbPointsY, int nbAPid);
    \brief  Remplit la matrice avec le fichier mesuresOrdonnees qui est le fichier contenant les RSS de références.

*/
int toMatrice(float* matrice, int nbPointsX, int nbPointsY, int nbAPid);


/**
    \fn void getSquareDistance(const float * matriceRef ,float* matricePos, int nbAP);
    \brief  retourne la distance euclidienne au carre entre matriceRef et matriceRef pour la dimension définie par nbAP.

*/

float getSquareDistance(const float * matriceRef ,float* matricePos, int nbAP);

/**
    \fn void alphaBetaFilter(float * x_m, float * y_m, float Dt);
    \brief Applique le filtre alpha beta sur *x_m et *y_m. Si Dt == -1 
			le filtre alpha beta est initialsé avec (*x_m, *y_m) comme position initiale
			et (0,0) comme vitesse initiale.

*/
void alphaBetaFilter(float * x_m, float * y_m, float Dt);



/**
    \fn void changeFreq(int freq)
    \brief Changer la frequence du mode monitore.
*/

void changeFreq(int freq);


/**
    \fn int connectToServer(int port, const char* server_add)
    \brief Se connecter au serveur
    \return socket's file descriptor
*/
int connectToServer(int port, const char* server_add);


/**
    \fn int setPostition(float *x, float *y, int fd);
    \brief Reçoit les positions du serveur et les transme en float
    \return -1 si il y a une erreur
*/
int setPostition(float *x, float *y, int fd);




#endif
