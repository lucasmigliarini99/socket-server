
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"
#include "../game/game.h"
#include "../config/readerIniGame.h"



Jeu games[2];

connection_t* connections[MAXSIMULTANEOUSCLIENTS];
int players[MAXSIMULTANEOUSCLIENTS];



void init_sockets_array_Server() {
    for (int i = 0; i < MAXSIMULTANEOUSCLIENTS; i++) {
        connections[i] = NULL;
    }
}

void add_Server(connection_t *connection) {
    for (int i = 0; i < MAXSIMULTANEOUSCLIENTS; i++) {
        if (connections[i] == NULL) {
            connections[i] = connection;
            return;
        }
    }
    perror("Too much simultaneous connections");
    exit(-5);
}

void del_Server(connection_t *connection) {
    for (int i = 0; i < MAXSIMULTANEOUSCLIENTS; i++) {
        if (connections[i] == connection) {
            connections[i] = NULL;
            return;
        }
    }
    perror("Connection not in pool ");
    exit(-5);
}

/*
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&lock);
pthread_mutex_unlock(&lock);
 */

/**
 * Thread allowing server to handle multiple client connections
 * @param ptr connection_t 
 * @return 
 */
void *threadProcess_Server(void *ptr) {
    Joueur buffer_in;
    //Joueur *buffer_out= malloc(sizeof(Joueur));

    

    int len;
    connection_t *connection;

    if (!ptr) pthread_exit(0);
    connection = (connection_t *) ptr;
    printf("New incoming connection \n");

    add_Server(connection);

    
    
    //addPlayer(connection->index);

    //Welcome the new client
    printf("Welcomeeeee #%i\n", connection->index);
    printf("Welcome Joueur:%i\n",connection->name);

    //init_jeux();

    while ((len = read(connection->sockfd, &buffer_in, sizeof(Joueur))) > 0) {

        // if (strncmp(buffer_in, "bye", 3) == 0) {
        //     break;
        // }

        if (buffer_in.id == 0)
        {
            buffer_in.id = connection->index;
        }
        
        if (buffer_in.sockfd == 0)
        {
            buffer_in.sockfd = connection->sockfd;
        }


        printf("DEBUG-----------------------------------------------------------\n");
        printf("Player: %i\n",connection->name);
        printf("len : %i\n", len);
        printf("Buffer : %s\n",buffer_in.pseudo);
        printf("choix : %d\n",buffer_in.choix);
        printf("score : %d\n",buffer_in.score);
        printf("----------------------------------------------------------------\n");

        if (buffer_in.enjeu == 0)
        {
            send_wait(connections,buffer_in);
        }

        if (buffer_in.enjeu == 1)
        {
            send_structure_to_game(buffer_in);
        }


        for (int i = 0; i < 2; i++)
        {
            if(games[i].j1.choix != NULL && games[i].j2.choix != NULL)
            {
                games[i] = jouer(games[i].j1,games[i].j2);
                send_joueur(games[i].j1);
                sleep(1);
                send_joueur(games[i].j2);
                games[i].j1.choix = NULL;
                games[i].j2.choix = NULL;
            }
        }
        

        //clear input buffer
       // memset(buffer_in, '\0', sizeof(Joueur));
    }
    printf("Connection to client %i ended \n", connection->index);
    close(connection->sockfd);
    del_Server(connection);
    free(connection);
    pthread_exit(0);

}

void send_structure_to_game(Joueur buffer_in)
{
    for (int i = 0; i < 2; i++)
    {
        if (buffer_in.id == games[i].j1.id)
        {
            games[i].j1 = buffer_in; 
        }

        if (buffer_in.id == games[i].j2.id)
        {
            games[i].j2 = buffer_in; 
        }
    }
}


void send_wait(connection_t* connections[MAXSIMULTANEOUSCLIENTS],Joueur buffer_in){
  
    for (int i = 0; i < 2; i++)
    {
        if (buffer_in.id == games[i].j1.id)
        {
            games[i].j1 = buffer_in; 
        }

        if (buffer_in.id == games[i].j2.id)
        {
            games[i].j2 = buffer_in; 
        }
    }

    if (games[0].j1.sockfd != NULL && games[0].j2.sockfd != NULL)
    {
        games[0].j1.enjeu = 1;
        games[0].j2.enjeu = 1;
        printf("La partie 1 peut commencer !\n");
        send_joueur(games[0].j1);
        sleep(2);
        send_joueur(games[0].j2);
    }

    if (games[1].j1.sockfd != NULL && games[1].j2.sockfd != NULL)
    {
        games[1].j1.enjeu = 1;
        games[1].j2.enjeu = 1;
        printf("j3 %d\n", games[1].j1.id);
        printf("j4 %d\n", games[1].j2.id);
        printf("La partie 2 peut commencer !\n");
        send_joueur(games[1].j1);
        sleep(2);
        send_joueur(games[1].j2);
    }
}

send_joueur(Joueur j)
{
    write(j.sockfd, &j, sizeof(Joueur));
}


int create_server_socket() {
    int sockfd = -1;
    struct sockaddr_in address;
    int port = 3379;

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd <= 0) {
        fprintf(stderr, "%s: error: cannot create socket\n");
        return -3;
    }


    /* bind socket to port */
    address.sin_family = AF_INET;
    //bind to all ip : 
    //address.sin_addr.s_addr = INADDR_ANY;
    //ou 0.0.0.0 
    //Sinon  127.0.0.1
    address.sin_addr.s_addr = inet_addr("0.0.0.0");
    address.sin_port = htons(port);

    /* prevent the 60 secs timeout */
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof (reuse));

    /* bind */
    if (bind(sockfd, (struct sockaddr *) &address, sizeof (struct sockaddr_in)) < 0) {
        fprintf(stderr, "error: cannot bind socket to port %d\n", port);
        return -4;
    }
    get_party(games);
    for (int i = 0; i < 2; i++)
    {
        games[i].j1.sockfd = NULL;
        games[i].j2.sockfd = NULL;
    }
    return sockfd;
}

