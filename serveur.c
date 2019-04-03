#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

 // Besoin de -pthread pour compiler

int dS1;
int dSC1;
int dSC2;

int tailleMax;

void *c1v2()
{
  int taille;

  char* msg = (char*) malloc((tailleMax+1) * sizeof(char)); // recoit le message

  int fini = 0;
  
  while(fini == 0)
  {
    taille = recv(dSC1,msg,tailleMax,0);
    
    if (taille < 0)
    { //gestion des erreurs
      printf("Reception finale du message trop long \n");
      fini = 1;
    }
    else if (taille == 0)
    {
      printf("Client deconnecte : End Of Message \n");
      fini = 1;
    }
    else // pas de probleme lors de la reception, on envoie au client 2
    {
      
      if (taille == tailleMax) // pour etre sur d'afficher tout le message, et pas depasser
      {
	char *pos = strchr(msg,'\0');
	*pos = ' ';
	msg[tailleMax] = '\0';
      }

      //renvoie au client 2
      int rep = send(dSC2,msg,strlen(msg)+1,0);
      
      if (rep < 0)//gestion des erreurs
      {
	printf("Serveur : erreur lors du send de reponse : client plus disponible \n");
	fini = 1; // client plus disponible
      }
      else if (rep == 0)
      {
	printf("Serveur : Client deconnecte avant reponse \n");
	fini = 1;
      }
    }
  }
  return 0;
}


void *c2v1()
{
  int taille;

  char* msg = (char*) malloc((tailleMax+1) * sizeof(char)); // recoit le message

  int fini = 0;
  
  while(fini == 0)
  {
    taille = recv(dSC2,msg,tailleMax,0);
    
    if (taille < 0)
    { //gestion des erreurs
      printf("Reception finale du message trop long \n");
      fini = 1;
    }
    else if (taille == 0)
    {
      printf("Client deconnecte : End Of Message \n");
      fini = 1;
    }
    else // pas de probleme lors de la reception, on envoie au client 2
    {
      
      if (taille == tailleMax) // pour etre sur d'afficher tout le message, et pas depasser
      {
	char *pos = strchr(msg,'\0');
	*pos = ' ';
	msg[tailleMax] = '\0';
      }

      //renvoie au client 2
      int rep = send(dSC1,msg,strlen(msg)+1,0);
      
      if (rep < 0)//gestion des erreurs
      {
	printf("Serveur : erreur lors du send de reponse : client plus disponible \n");
	fini = 1; // client plus disponible
      }
      else if (rep == 0)
      {
	printf("Serveur : Client deconnecte avant reponse \n");
	fini = 1;
      }
    }
  }
  return 0;
}

void fin() //FONCTION QUI TRAITE LES CTRL+C
{
  printf("\n Au revoir \n");
  //Fermer
  close(dS1);
  exit(0);
}


int main(int argc, char* argv[]) // serveur
{  
  signal(SIGINT,fin);
  
  tailleMax = 2000;
  
  //Creer socket
  dS1 = socket(PF_INET,SOCK_STREAM,0); // dedoublé pour les 2 clients
  
  if (dS1 == -1)
  {
    printf("Serveur : probleme dans la creation de socket 1");
    raise(SIGINT);
  }
  
  //Nommer
  struct sockaddr_in ad1;
  ad1.sin_family = AF_INET;
  ad1.sin_addr.s_addr = INADDR_ANY;
  ad1.sin_port = htons(0);
  
  if (bind(dS1,(struct sockaddr*)&ad1,sizeof(ad1)) <0) //si erreur lors du bind
  {
    printf("Probleme binding le port 1\n");
    raise(SIGINT);
  }

  //Passer en mode ecoute
  if (listen(dS1,5)<0)// on limite a 5 la taille du buffer : si erreur lors du listen
  {
    printf("Serveur 1 : je suis sourd\n");
    raise(SIGINT);
  }

  socklen_t lgS1 = sizeof(struct sockaddr_in); 
  if (getsockname(dS1,(struct sockaddr*)&ad1,&lgS1)<0) // associe un port au socket ?
  {
    printf("Erreur du getsockname 1\n");
    raise(SIGINT);
  }
  
  printf("Serveur : mon numero de port est : %d \n",  ntohs(ad1.sin_port));
  
  //Attendre un clients

  char* msg = (char*) malloc((tailleMax+1) * sizeof(char)); // recoit le message
  
  while(1)
  {
    strcpy(msg,"Vous etes client 1");
    
    printf("J'attend un client 1\n");
    struct sockaddr_in aC1; // adresse client
    socklen_t lg1 = sizeof(struct sockaddr_in);
    dSC1 = accept(dS1,(struct sockaddr*)&aC1,&lg1); // accepter la connexion client
    
    if (dSC1 < 0)
    {//gere les erreurs
      printf("Probleme d'acceptation 1");
      raise(SIGINT);
    }
    
    printf("Client 1 connecté : %s:%d \n",inet_ntoa(aC1.sin_addr), ntohs(aC1.sin_port));

        
    printf("J'attend un client 2\n");
    struct sockaddr_in aC2; // adresse client
    socklen_t lg2 = sizeof(struct sockaddr_in);
    dSC2 = accept(dS1,(struct sockaddr*)&aC2,&lg2); // accepter la connexion client
    
    if (dSC2 < 0)
    {//gere les erreurs
      printf("Probleme d'acceptation 2");
      raise(SIGINT);
    }
    
    printf("Client 2 connecté : %s:%d \n",inet_ntoa(aC2.sin_addr), ntohs(aC2.sin_port));

    // FAIRE LES PTHREAD
    pthread_t tc1v2;
    pthread_t tc2v1;
    
    pthread_create(&tc1v2,0,c1v2,0);
    pthread_create(&tc2v1,0,c2v1,0);

    // PTRHEAD WAIT FIN DES CLIENTS !!
    
    pthread_join(tc1v2,0);
    pthread_join(tc2v1,0);

    //fermeture socket client
    
    printf("Fermeture du dialogue avec le client 1\n");
    close(dSC1);  
    printf("Fermeture du dialogue avec le client 2\n");
    close(dSC2);
    
  }

}
