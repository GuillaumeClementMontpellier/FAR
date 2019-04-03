#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) // client
{
  int tailleMax = 2000;
  
  if (argc < 2)
  {
    printf("Probleme d'arguments\nFormat : ./client \"IP\" \"port\" (facultatif : <\"nomFichierSource\")\n");
    return 1;
  }
  
  //Creer socket
  int dS = socket(PF_INET,SOCK_STREAM,0);
  
  //Demander une connexion
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur/serveur
  
  inet_pton(AF_INET,argv[1],&(ad.sin_addr)); //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  socklen_t lgA = sizeof(struct sockaddr_in);
  connect(dS,(struct sockaddr *)&ad,lgA);
  
  //Communiquer
  //crée la variable pour l'envoi
  char* msg = (char*) malloc(tailleMax * sizeof(char));
  msg[0]='\0';

  int fini = 0;

  while(fini==0) // tant que pas fin
  {

    
    //Etape : recevoir la reponse (nombre d'octet) pas besoin ici
    int rep = recv(dS,msg,tailleMax,0);
    if (rep < 0)
    { //gestion des erreurs
      printf("Erreur reception\n");
    }
    else if (rep == 0)
    {
      printf("Serveur deconnecte \n");
    }

    printf("Vous recevez ce message : %s\n",msg);
    
    if (strcmp(msg,"fin")==0) // check fin
    {
      fini = 1;
    }
    
    if(fini == 0) // si le message recu n'est pas fin
    {

      printf("Entrez un message à envoyer :");
    
      fgets(msg,tailleMax,stdin);//message 1
      char *pos = strchr(msg,'\n');
      *pos = '\0';
      
      //Etape : envoyer
      send(dS,msg,strlen(msg)+1,0); // +1 pour envoyer le \0
      
      if (strcmp(msg,"fin")==0) // check fin
      {
	fini = 2;
      }
      
    }
  }

  free(msg);
  
  //Fermer
  close(dS);

}
