/***************************************************
* file:     main.cpp
* Author:  Tristan Brousseau-Rigaudie
*
* Ensemble des fonctionalités de ce code :
*
  *
    * Capture de la force du signal de points d'accès,
    * Localisation par empreinte
    * Lissage et suivi à l'aide d'un filtre alpha-Beta
    * Envoit des données à un serveur.
    *
  *
*
*
*
* Les lignes utilisant lpcap on été prises de Martin Casado
* Fichier original : testpcap3.c
* À partir de  : http://yuba.stanford.edu/~casado/pcap/section3.html
*****************************************************/

#include <string.h>
#include <pcap.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> 
#include <inttypes.h> // print int8_t
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <cstdio>
#include <math.h>
#include <fstream>
#include "localisation.h"

using namespace std;




#define SERVER_ADD "10.200.0.9"

/** Nombre d' AP disponible*/
int NbAP = 4;
/** Nombre de paquet a capter par AP a la fois*/
int NbPacketByAP = 2;
/** Nombre de point en X a couvrir*/
int NB_POINTS_X = 8;
/** Nombre de point en Y a couvrir*/
int NB_POINTS_Y = 8;
/** La valeur en metre qu'un point en X ou en Y equivaut */
int SCALE  = 2;




/** Les differents filtres selon les quatre points d ' acces (les MAC
*   address de chaque point d ' a c c e s ) 
*/
char filter[120] = "ether host f4:0f:1b:ab:28:f9\0ether host f8:c2:88:14:b5:39\0ether host f8:c2:88:2a:e0:c9\0ether host f8:c2:88:22:09:79\0";


/*Channel 1, 6, 11*/

int freqArray[3]= {2412,2437,2462};





int main(int argc, char **argv)
{

    /** Parametres pour LPCAP */ 
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr hdr;     /* pcap.h                    */
    struct ether_header *eptr;  /* net/ethernet.h            */
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    struct pcap_pkthdr header;
    const u_char *packet;
	pcap_t* descr;
    int rc = 0;
    int index;

    /** Parametres de connexion au serveur */
    char* serverAdd;
    int fd;
    int arrayRSS[(50+2)*NbAP+1];
    int len;
    int portNum = 80;
    char rcvBuff[2];

    /** parametre pour la localisation 3NN */
    float matricePos[NbAP];
    float matrice[NB_POINTS_X][NB_POINTS_Y][NbAP];
    float pos_x[2] = {0,0};
    float pos_y[2] = {0,0};
    int pointA[2] = {0,0};
    int pointB[2] = {0,0};
    int pointC[2] = {0,0};
    int tampon = 0;

    /** Pour determiner le temps de calcul */
    struct timespec start, end, beginning;

    /** Pour analyser les résultats */
    ofstream f_out_Pos;
    ofstream f_out_mesures;
    f_out_Pos.open("AlphaBetaOutPut.csv", ios::app);

    if(argc != 2){
        printf("Usage: %s \"AdresseIPServeur\"\n",argv[0]);
        serverAdd=SERVER_ADD;
    }
    else{
		serverAdd = argv[1];
    }

    printf("connecting to server \n");

	/** Connection au serveur*/
    do{
        fd = connectToServer(portNum, serverAdd);
    }while(fd < 0);

    printf("connection to server DONE \n");


	/** Initialisation de la matrice de reference*/
    if(toMatrice(&matrice[0][0][0], NB_POINTS_X, NB_POINTS_Y, NbAP) == -1){
        printf("ERROR toMatrice");
        return -1;
    }



    /** Initialisation de pcap */
    dev = pcap_lookupdev(errbuf);

    if(dev == NULL)
    {
        printf("%s\n",errbuf);
        exit(1);
    }

    strcpy(dev, "mon7");
    printf("DEV: %s\n",dev);
    descr = pcap_open_live(dev,BUFSIZ,0,500,errbuf);

    if(descr == NULL)
    {
        printf("pcap_open_live(): %s\n",errbuf);
        exit(1);
    }

    /** Le temps de depart */
    clock_gettime( CLOCK_REALTIME, &beginning);

    while(1){
        clock_gettime( CLOCK_REALTIME, &start);
        index = 1;
		int index_NbPacket = 0;
		int nbPackets = 0; 
    	arrayRSS[index++] = NbAP; 
    	changeFreq(freqArray[0]);
		/** RECEPTION de commande*/
        len = recv(fd,rcvBuff,2,0);
		if(len < 0){
                printf("error recv ptSize : %d \n", len);
                return -1;
        }
        /** Traitement de la commande*/
		switch(rcvBuff[0]){
			case 'F':
				printf("F \n");
				close(fd);
				return 0;
					case 'I':
				NbPacketByAP =2;
				if(setPostition(pos_x, pos_y, fd) == -1) return -1;

						break;
			case 'M' :
				// Mesures
				f_out_mesures.open("RSS.txt", ios::app);
						if(setPostition(pos_x, pos_y, fd) == -1) return -1;	
				NbPacketByAP = 50;
						f_out_mesures << "x"<< (int)pos_x[0] << "y"<<(int)pos_y[0] <<endl;
				break;

			case 'S' :
				// Stop Mesures
				if(f_out_mesures.is_open()){
					f_out_mesures.close();
				}
				NbPacketByAP =2 ;
				break;
				
			default:
				// On continue
				printf("C \n");
				break; /*case 'C'*/
						
		}


        // On attrappe tous les paquets des AP pour cette periode

        for(int i = 0; i < NbAP; i++){
			if(i > 1){
					changeFreq(freqArray[i-1]);
			}

			nbPackets = 0;

			printf("Capture avec  APid : %d, filtre :  %s \n", i, filter+(i*29));

			arrayRSS[index++] = i; // MAC id;

	        index_NbPacket = index++;
	    	/** Ajout d'un filtre au captage de trame*/
	    	if(pcap_compile(descr,&fp,filter+(i*29),0,netp) == -1)
	    	{ fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }
	    	if(pcap_setfilter(descr,&fp) == -1)
	    	{ fprintf(stderr,"Error setting filter\n"); exit(1); }

			/** On capte le nombre de paquet désiré */
	    	for(int j = 0; j < NbPacketByAP; j++){
				packet = pcap_next(descr, &header);
				if(packet > 0 ){
					nbPackets++;
					arrayRSS[index++] = (int)*(((int8_t *)packet)+14);
				}
			}
			arrayRSS[index_NbPacket] = nbPackets;
    	}
		printf("\n Capture FINI \n Grosseur du paquet : %d bytes", index);

        arrayRSS[0] =index -1;

    	/** On decripte le tableau arrayRSS*/
		index =1;
		int NbMAC = (int)arrayRSS[index++];
	
        //printf("\n Resume \n\n \tNb de AP : %d  \n", NbMAC);
	
        for(int j = 0; j < NbMAC; j ++){

            int i =0;
            float moyenne = 0;
            int nbRSS;

            if(f_out_mesures.is_open()){
				// En mode mesure
            	f_out_mesures << "AP"<<j<<endl;
            }

			index++;
            nbRSS = (int)arrayRSS[index++];

            //printf("\tNb rss   : %d \n", nbRSS);

            if(nbRSS !=0 ){

				for(i =0; i<nbRSS-1; i++){
					if(f_out_mesures.is_open()){
						f_out_mesures <<(int)arrayRSS[index] << ",";
					}
                    moyenne += (float)arrayRSS[index++];
                }
				if(f_out_mesures.is_open()){
					f_out_mesures <<(float)arrayRSS[index] << ";"<<endl;
				}
                moyenne += arrayRSS[index++];
                moyenne = moyenne/(float)nbRSS;

            }else moyenne = -80;

            printf("\t\tMoyenne %.2f \n", moyenne);
            matricePos[j] = moyenne;
        }

		// Algorithme 3-NN
		
        float distanceA = 333332.0;
        float distanceB = 333332.0;
        float distanceC = 333332.0;
        float distance;
        float total3Stats = 0;


		//Iteration sur la matrice de reference pour déterminer les 3 plus petites distances
        for(int x = 0 ; x < NB_POINTS_X; x++){
            for(int y = 0; y< NB_POINTS_Y; y++){
                if((int)matrice[x][y][0] != -1){

                    distance = getSquareDistance(&matrice[x][y][0], matricePos, NbAP);

                    // on regarde si les 3 NN ont changés

                    if(distance < distanceA){
                        // On change le centre de recherche
                        distanceC = distanceB;
                        distanceB = distanceA;
                        distanceA = distance;


                        pointC[0] = pointB[0];
                        pointC[1] = pointB[1];

                        pointB[0] = pointA[0];
                        pointB[1] = pointA[1];
                        pointA[0] = x;
                        pointA[1] = y;


                    }else if(distance < distanceB){
                        distanceC = distanceB;
                        distanceB = distance;


                        pointC[0] = pointB[0];
                        pointC[1] = pointB[1];

                        pointB[0] = x;
                        pointB[1] = y;

                    }else if(distance < distanceC){
                        distanceC = distance;


                        pointC[0] = x;
                        pointC[1] = y;

                    }


                }


            }

        }


        // 3NN
        pos_x[0] = SCALE*((float)(pointA[0]+pointB[0]+pointC[0])/3.0);
        pos_y[0] = SCALE*((float)(pointA[1]+pointB[1]+pointC[1])/3.0);

        	
		/*
		// weighted 3NN	
		
		if(distanceA == 0.0){
            // une securité -- il y a des chances que le code se rende jamais ici.
            distanceA = 0.01;
        }

        total3Stats = 1/sqrtf(distanceA) +1/sqrtf(distanceB)+ 1/sqrtf(distanceC);

        pos_x[1] = SCALE*((float)pointA[0]/sqrtf(distanceA) + (float)pointB[0]/sqrtf(distanceB) + (float)pointC[0]/sqrtf(distanceC))total3Stats;
        pos_y[1] = SCALE*((float)pointA[1]/sqrtf(distanceA) + (float)pointB[1]/sqrtf(distanceB) + (float)pointC[1]/sqrtf(distanceC))/total3Stats;
		
		 */
        clock_gettime( CLOCK_REALTIME, &end);
        float Dt;
        Dt = (end.tv_sec - beginning.tv_sec) + (float)(end.tv_nsec - beginning.tv_nsec)/1000000000.0d;

        f_out_Pos <<  Dt <<", Position, (3NNx ; 3NNy), (w3NNx; w3NNy), filter(3NNx; 3NNy) ";

        f_out_Pos <<","<< pos_x[0] <<","<<pos_y[0]<<","<< pos_x[1] <<","<< pos_y[1];

        Dt = (end.tv_sec - start.tv_sec) + (float)(end.tv_nsec - start.tv_nsec)/1000000000.0d;
        // On applique le filtre
        alphaBetaFilter(pos_x, pos_y, Dt);

        f_out_Pos <<","<< pos_x[0] <<","<< pos_y[0] << endl;


        // ENVOI
	
		char envoi[40];
		memset(envoi, '\0', 40);
		printf("x %.2f, %.2f , y %.2f, %.2f \n",pos_x[0], pos_x[1],pos_y[0], pos_y[1]);
	
		int n = 0;
		n+= sprintf(envoi,"%.2fs", pos_x[0]);	
		n+=sprintf(envoi+n,"%.2fs", pos_x[1]);
        n+=sprintf(envoi+n,"%.2fs",pos_y[0]);
        n+=sprintf(envoi+n,"%.2fs", pos_y[1]);
        len = send(fd, envoi, n,0);
        if(len < 0){
              fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

              exit(EXIT_FAILURE);
        }

	}

    close(fd);
    return 0;
}
