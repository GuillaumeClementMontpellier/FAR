#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//FONCTION QUI TRAITE LES CTRL+C

int dS1;
int dS2;

void fin()
{
  printf("\n Au revoir \n");
  //Fermer
  close(dS1);
  close(dS2);
}


int main(int argc, char* argv[]) // serveur
{  
  signal(SIGINT,fin);
  
  const int tailleMax = 2000;
  
  //Creer socket
  dS1 = socket(PF_INET,SOCK_STREAM,0); // dedoublé pour les 2 clients
  dS2 = socket(PF_INET,SOCK_STREAM,0);
  
  if (dS1 == -1)
  {
    printf("Serveur : probleme dans la creation de socket 1");
    return 1;
  }
  if (dS2 == -1)
  {
    printf("Serveur : probleme dans la creation de socket 2");
    return 1;
  }
  
  //Nommer
  struct sockaddr_in ad1;
  ad1.sin_family = AF_INET;
  ad1.sin_addr.s_addr = INADDR_ANY;
  ad1.sin_port = htons(0);
  struct sockaddr_in ad2;
  ad2.sin_family = AF_INET;
  ad2.sin_addr.s_addr = INADDR_ANY;
  ad2.sin_port = htons(0);
  
  if (bind(dS1,(struct sockaddr*)&ad1,sizeof(ad1)) <0) //si erreur lors du bind
  {
    printf("Probleme binding le port 1\n");
    close(dS1);
    return 1;
  }
  if (bind(dS2,(struct sockaddr*)&ad2,sizeof(ad2)) <0) //si erreur lors du bind
  {
    printf("Probleme binding le port 2\n");
    close(dS2);
    return 1;
  }

  //Passer en mode ecoute
  if (listen(dS1,5)<0)// on limite a 5 la taille du buffer : si erreur lors du listen
  {
    printf("Serveur 1 : je suis sourd\n");
    close(dS1);
    return 1;
  }
  if (listen(dS2,5)<0)// on limite a 5 la taille du buffer : si erreur lors du listen
  {
    printf("Serveur 2 : je suis sourd\n");
    close(dS2);
    return 1;
  }

  socklen_t lgS1 = sizeof(struct sockaddr_in); 
  if (getsockname(dS1,(struct sockaddr*)&ad1,&lgS1)<0) // associe un port au socket ?
  {
    printf("Erreur du getsockname 1\n");
    close(dS1);
    return 1;
  }
  socklen_t lgS2 = sizeof(struct sockaddr_in); 
  if (getsockname(dS2,(struct sockaddr*)&ad2,&lgS2)<0) // associe un port au socket ?
  {
    printf("Erreur du getsockname 2\n");
    close(dS2);
    return 1;
  }
  
  printf("Serveur : mon numero de port 1 est : %d \n",  ntohs(ad1.sin_port));
  printf("Serveur : mon numero de port 2 est : %d \n",  ntohs(ad2.sin_port));

  //printf("Appuyez sur Entree pour lancer le serveur\n");
  
  //char AttenteReponse = fgetc(stdin); // decommenter cette ligne est la precedente pour pouvoir attendre apres le lancement
  
  //Attendre un clients
  char* msg = (char*) malloc((tailleMax+1) * sizeof(char)); // recoit le message
  
  while(1)
  {
    strcpy(msg,"Vous etes client 1");
    
    printf("J'attend un client 1\n");
    struct sockaddr_in aC1; // adresse client
    socklen_t lg1 = sizeof(struct sockaddr_in);
    int dSC1 = accept(dS1,(struct sockaddr*)&aC1,&lg1); // accepter la connexion client
    
    if (dSC1 < 0)
    {//gere les erreurs
      printf("Probleme d'acceptation 1");
      return 1;
    }
    
    printf("Client 1 connecté : %s:%d \n",inet_ntoa(aC1.sin_addr), ntohs(aC1.sin_port));

    
    int rep = send(dSC1,msg,strlen(msg),0);
    if (rep < 0)//gestion des erreurs
    {
      printf("Serveur : erreur lors du send du message : client pas disponible \n");
    }
    else if (rep == 0)
    {
      printf("Serveur : Client deconnecte avant reponse \n");
    }
    
    printf("J'attend un client 2\n");
    struct sockaddr_in aC2; // adresse client
    socklen_t lg2 = sizeof(struct sockaddr_in);
    int dSC2 = accept(dS2,(struct sockaddr*)&aC2,&lg2); // accepter la connexion client
    
    if (dSC2 < 0)
    {//gere les erreurs
      printf("Probleme d'acceptation 2");
      return 1;
    }
    
    printf("Client 2 connecté : %s:%d \n",inet_ntoa(aC2.sin_addr), ntohs(aC2.sin_port));

    int fini = 0;
    int taille = 0;
    
    while (fini == 0){
      
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
      else // pas de probleme lors de la receptioni
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

      //puis pareil pour client 2 : echange client 1 et 2
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
      else // pas de probleme lors de la receptioni
      {
	
	if (taille == tailleMax) // pour etre sur d'afficher tout le message, et pas depasser
	{
	  char *pos = strchr(msg,'\0');
	  *pos = ' ';
	  msg[tailleMax] = '\0';
	}

	//renvoie au client 1
	int rep = send(dSC1,msg,strlen(msg)+1,0); // +1 pour le \0
	
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

    //fermeture socket client
    
    printf("Fermeture du dialogue avec le client 1\n");
    close(dSC1);  
    printf("Fermeture du dialogue avec le client 2\n");
    close(dSC2);
    
  }
	
  free(msg);
  
  printf("\n Au revoir \n");
  //Fermer
  close(dS1);
  close(dS2);

}
