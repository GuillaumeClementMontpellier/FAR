#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) // serveur
{
  const int tailleMax = 124;
  
  //Creer socket
  int dS = socket(PF_INET,SOCK_STREAM,0);
  
  if (dS == -1)
  {
    printf("Serveur : probleme dans la creation de socket");
    return 1;
  }
  
  //Nommer
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(0);
  
  if (bind(dS,(struct sockaddr*)&ad,sizeof(ad)) <0) //si erreur lors du bind
  {
    printf("Probleme binding le port\n");
    close(dS);
    return 1;
  }

  //Passer en mode ecoute
  if (listen(dS,5)<0)// on limite a 5 la taille du buffer : si erreur lors du listen
  {
    printf("Serveur : je suis sourd\n");
    close(dS);
    return 1;
  }

  socklen_t lgS = sizeof(struct sockaddr_in); 
  if (getsockname(dS,(struct sockaddr*)&ad,&lgS)<0) // associe un port au socket ?
  {
    printf("Erreur du getsockname\n");
    close(dS);
    return 1;
  }
  
  printf("Serveur : mon numero de port est : %d \n",  ntohs(ad.sin_port));

  //printf("Appuyez sur Entree pour lancer le serveur\n");
  
  //char AttenteReponse = fgetc(stdin); // decommenter cette ligne est la precedente pour pouvoir attendre apres le lancement
  
  //Attendre un clients
  while(1)
  {
    printf("J'attend un client\n");
    struct sockaddr_in aC; // adresse client
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSC = accept(dS,(struct sockaddr*)&aC,&lg); // accepter la connexion client
    
    if (dSC < 0)
    {//gere les erreurs
      printf("Probleme d'acceptation");
      return 1;
    }
    
    printf("Client connecté : %s:%d \n",inet_ntoa(aC.sin_addr), ntohs(aC.sin_port));

    int fini = 1;
    int nbrRecu = 0;
    //int dispo = 1;
    
    while (fini > 0){
      
      //recevoir le message
      char* msg = (char*) malloc((tailleMax+1) * sizeof(char));
      /*
      int i = 0; // pour etre sur de nettoyer la memoire !
      for(;i<tailleMax;i++)
      {
	msg[i] = ' ';
      }
      msg[tailleMax] = '\0'; */
      
      int taille = recv(dSC,msg,tailleMax,0);
      
      if (taille < 0)
      { //gestion des erreurs
	printf("Reception finale du message trop long \n");
	fini = 0;
      }
      else if (taille == 0)
      {
	printf("Client deconnecte : End Of Message \n");
	fini = 0;
      }
      else // pas de probleme d'envoi
      {
	nbrRecu += taille;
	
	if (taille == tailleMax) // pour etre sur d'afficher tout le message, et pas depasser
	{
	  char *pos = strchr(msg,'\0');
	  *pos = ' ';
	  msg[tailleMax] = '\0';
	  }
	
	printf("Taille du message : %d octets\nMessage : %s\n",taille,msg);
	
	free(msg);

	/*
	if(dispo == 1)
	{
	  //reponse
	  int rep = send(dSC,&taille,sizeof(int),0);
	  
	  if (rep < 0)//gestion des erreurs
	  {
	    printf("Serveur : erreur lors du send de reponse : client plus disponible \n");
	    dispo = 0; // client plus disponible
	  }
	  else if (rep == 0)
	  {
	    printf("Serveur : Client deconnecte avant reponse \n");
	    fini = 0;
	  }
	} */ // pas besoin de reponse ici, MAIS FAIT FINIR TROP TOT
      }
    }

    printf("Taille totale de cette communication : %d\n",nbrRecu);
    
    //fermeture socket client
    
    printf("Fermeture du dialogue avec le client\n");
    close(dSC);  
  }
  
  printf("\n Au revoir \n");
  //Fermer
  close(dS);

}
