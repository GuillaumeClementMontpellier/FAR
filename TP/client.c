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
  
  if (argc < 3)
  {
    printf("Probleme d'arguments\nFormat : ./client \"IP\" \"port\" \"nbrIteration\" (facultatif : <\"nomFichierSource\")\n");
    return -1;
  }
  
  //Creer socket
  int dS = socket(PF_INET,SOCK_STREAM,0);

  //crée la variable pour l'envoi
  char* msg = (char*) malloc(tailleMax * sizeof(char));
  int nbrEnvoye = 0;
  
  fgets(msg,tailleMax,stdin);//message 1
  char *pos = strchr(msg,'\n');
  *pos = '\0';

  //Demander une connexion
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur
  inet_pton(AF_INET,argv[1],&(ad.sin_addr)); //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  socklen_t lgA = sizeof(struct sockaddr_in);
  connect(dS,(struct sockaddr *)&ad,lgA);
  
  //Communiquer
  int i = atoi(argv[3]);
  
  printf("Envoi de %d message(s)\n",i);
  
  while( i > 0 )
  {
    //Etape : envoyer
    int rep = send(dS,msg,strlen(msg)+1,0); // envoie le \0
    
    if(rep > 0)
    {
      nbrEnvoye += strlen(msg)+1;
    }
    else
    {
      printf("What ?");
    }
    /*
    //Etape : recevoir la reponse (nombre d'octet) pas besoin ici
    int r;
    rep = recv(dS,&r,sizeof(int),0);
    if (rep < 0)
    { //gestion des erreurs
      printf("Erreur reception\n");
    }
    else if (rep == 0)
    {
      printf("Serveur deconnecte \n");
    }
    */
    i--;
    
  }
  printf("Fin de l'envoi des message(s)\n");
  
  /*
  fgets(msg,tailleMax,stdin);//message 2
  pos = strchr(msg,'\n');
  *pos = '\0';
  send(dS,msg,strlen(msg),0);//strlen+1 pour envoyer le \O dans lemessage, laissant ainsi bien se terminer l'affichage
  nbrEnvoye += strlen(msg);*/

  free(msg);

  printf("nbr Octets envoyés : %d\n",nbrEnvoye); 
  
  //Fermer
  close(dS);

}
