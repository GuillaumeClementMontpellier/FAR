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

int dS;

int tailleMax;

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
      printf("Erreur reception\n");
    }
    else if (rep == 0)
    {
      printf("Serveur deconnecte \n");
    }
    
    printf("Votre correspondant vous dit : %s\n",msg);
    
    if (strcmp(msg,"fin")==0) // check fin
    {
      free(msg);
      raise(SIGUSR1); // fini le programme
    }
  }
    
}
void *envoyer()
{
  //crée la variable pour l'envoi
  char* msg = (char*) malloc(tailleMax * sizeof(char));
  msg[0]='\0';
  
  while(1)
  { 
    fgets(msg,tailleMax,stdin); //message 1
    char *pos = strchr(msg,'\n');
    *pos = '\0';
    
    //Etape : envoyer
    send(dS,msg,strlen(msg)+1,0); // +1 pour envoyer le \0
    
    if (strcmp(msg,"fin")==0) // check fin
    {
      free(msg);
      raise(SIGUSR1);
    }
     
  }
}

void fin()
{
  close(dS);
  exit(0);
}

int main(int argc, char* argv[]) // client
{  
  signal(SIGUSR1,fin); // signal pour finir
  
  if (argc < 2)
  {
    printf("Probleme d'arguments\nFormat : ./client \"IP\" \"port\" (facultatif : <\"nomFichierSource\")\n");
    return 1;
  }
  
  //Creer socket
  dS = socket(PF_INET,SOCK_STREAM,0);

  tailleMax = 2000;
  
  //Demander une connexion
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur/serveur
  
  inet_pton(AF_INET,argv[1],&(ad.sin_addr)); //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  socklen_t lgA = sizeof(struct sockaddr_in);
  connect(dS,(struct sockaddr *)&ad,lgA);
  
  //Communiquer

  printf("Bienvenue dans cette messagerie\n");

  //Separer la lecture et l'ecriture en fonction, puis les appeler en thread => plus de 'fini' => signal pour finir
  
  pthread_t tRecevoir;
  pthread_t tEnvoyer;

  pthread_create(&tRecevoir,0,recevoir,0);
  pthread_create(&tEnvoyer,0,envoyer,0);

  pthread_join(tRecevoir,0);
  pthread_join(tEnvoyer,0);

  return 0;
  
}
