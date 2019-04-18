#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

//a besoin de -pthread pour compiler

int dS; // socket

int tailleMax; // taille max des messages pour fgets

char* pseudo;

void *recevoir()
{
  //crée la variable pour l'envoi
  char* msg = (char*) malloc(tailleMax * sizeof(char));
  msg[0]='\0';
  
  while(1)
  {
    //Etape : recevoir la reponse (nombre d'octet) pas besoin ici
    int rep = recv(dS,msg,tailleMax,0);
    
    if (rep < 0)
    { //gestion des erreurs
      perror("Client : Recevoir : recv -1 ");
      free(msg);
      raise(SIGUSR1);
    }
    else if (rep == 0)
    {
      perror("Client : Recevoir : recv 0 ");
      free(msg);
      raise(SIGUSR1);
    }
    
    printf("%s\n",msg);
    
    /* Ne s'arrete plus si recoit fin, seulement si on envoie fin
    if (strcmp(msg,"fin")==0) // check fin
    {
      fin = 1;
    } 
    */
  }
    
}
void *envoyer()
{
  //crée la variable pour l'envoi
  char* msg = (char*) malloc(tailleMax * sizeof(char));
  char *buf = (char*) malloc(tailleMax * sizeof(char));
  msg[0]='\0';
  
  while(1)
  { 
    fgets(msg,tailleMax-strlen(pseudo),stdin); //message 1
    char *pos = strchr(msg,'\n');
    *pos = '\0';

    //Colle le pseudo au debut du message
    strcpy(buf,pseudo);
    strcat(buf," : ");
    strcat(buf,msg);    
    
    //Etape : envoyer
    send(dS,buf,strlen(buf)+1,0); // +1 pour envoyer le \0
    
    if (strcmp(msg,"fin")==0) // check fin
    {
      free(msg);
      free(buf);
      raise(SIGUSR1);
    }
     
  }
}

void fin ()
{
  close(dS);
  exit(0);
}

int main(int argc, char* argv[]) // client
{
  signal(SIGUSR1,fin);
  
  if (argc < 4)
  {
    printf("Probleme d'arguments\nFormat : ./client \"IP\" \"port\" \"pseudo\"\n");
    return 1;
  }

  pseudo = argv[3];
  
  //Creer socket
  dS = socket(PF_INET,SOCK_STREAM,0);

  tailleMax = 2000;
  
  //Demander une connexion au serveur
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur/serveur
  
  inet_pton(AF_INET,argv[1],&(ad.sin_addr)); //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  socklen_t lgA = sizeof(struct sockaddr_in);
  
  connect(dS,(struct sockaddr *)&ad,lgA);
    
  //Communiquer

  printf("Bienvenue dans cette messagerie\n");

  //Separer la lecture et l'ecriture en fonction, puis les appeler en thread
  
  pthread_t tRecevoir;
  pthread_t tEnvoyer;

  pthread_create(&tRecevoir,0,recevoir,0);
  pthread_create(&tEnvoyer,0,envoyer,0);

  pthread_join(tRecevoir,0);
  pthread_join(tEnvoyer,0);
  
  close(dS);

  return 0;
  
}
