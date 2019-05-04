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
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

//a besoin de -pthread pour compiler

int dS; // descripteur de socket du serveur

struct sockaddr_in ad;

int dSRec; //descripteur pour reception (en UDP) => besoin de bind, même addr que dS

struct sockaddr_in adUDP;

int tailleMax; // taille max des messages pour fgets

char *pseudo;

void *recevoir_file()
{
  printf("Debut reception file\n");
  
  // Recoit Taille Nom
  int taille;
  recvfrom(dSRec,&taille,sizeof(int),0,NULL,NULL);

  char *filename = (char *) malloc((taille+1) * sizeof(char));

  // Recoit Nom Fichier
  recvfrom(dSRec,filename,taille,0,NULL,NULL);

  // Ouvre le fichier pour ecrire
  char *filepath = (char*)malloc(tailleMax * sizeof(char));
  strcpy(filepath,"./fileDest/");
  strcat(filepath,filename);
  FILE *file = fopen(filepath,"wb");

  // Recoit Taille fichier
  int filesize;
  recvfrom(dSRec,&filesize,sizeof(int),0,NULL,NULL);

  char * buf = (char*) malloc(filesize * sizeof(char));

  // Recoit fichier (et ecrit ?)
  recvfrom(dSRec,buf,filesize,0,NULL,NULL);

  //TODO : ecrire fichier
  printf("Reception de %s, enregistre dans %s, de taille %d\n",filename,filepath,filesize);

  fwrite(buf,filesize,1,file);
  
  // Exit thread
  free(filepath);
  free(filename);
  free(buf);
  fclose(file);
  
  printf("Fin reception file\n");

  pthread_exit(0);
  
}

void *envoyer_file(void * adrDest)
{
  //recuperer addr et allouer place pour filename et filepath
  struct sockaddr_in addrDest = *((struct sockaddr_in *) adrDest);
  
  char *filepath = (char*) malloc(tailleMax * sizeof(char));
  strcpy(filepath,"./fileSource/");

  char *filename = (char*) malloc(tailleMax * sizeof(char));  
  
  // Cree Socket UDP
  int dSEnv = socket(PF_INET,SOCK_DGRAM,0);
  
  if (dSEnv == -1)
  {
    perror("Socket UDP ");
  }
  
  socklen_t l = sizeof(struct sockaddr_in);

  // lister les fichiers disponibles
  DIR *dp;
  struct dirent *ep;
  
  dp = opendir(filepath);
  
  if (dp != NULL) {
    printf("Voilà la liste de fichiers :\n");
    ep = readdir (dp);
    while (ep) {
      if(strcmp(ep->d_name,".")!=0 && strcmp(ep->d_name,"..")!=0)
      {
	printf("%s\n",ep->d_name);
      }
      ep = readdir (dp);
    }    
    (void) closedir (dp);
  }
  else {
    perror ("Ne peux pas ouvrir le répertoire");
  }

  // Demande le nom du fichier
  printf("Nom du fichier à envoyer : ");
  fgets(filename,(tailleMax-strlen(filepath)),stdin);
  char *pos = strchr(filename,'\n');
  *pos = '\0';

  strcat(filepath,filename);

  // ouvre le fichier
  FILE *file = fopen(filepath,"rb");
  if (file == NULL)
  {
    perror("fopen");
  }

  // repere la taille du fichier
  struct stat res;
  stat(filepath,&res);
  int filesize = res.st_size;

  char *bufEnvoi = (char*) malloc(filesize * sizeof(char));
  bufEnvoi[0] = '\0';

  char buf[2];
  buf[1] = '\0';

  //copier le fichier dans le buffer
  buf[0] = fgetc(file);
  while( buf[0] != EOF)//continue tant que pas EOF
  {
    strcat(bufEnvoi,buf);
    buf[0] = fgetc(file);
  }

  // Envoie Taille Nom a dest
  int taille = (strlen(filename)+1);
  sendto(dSEnv,&taille,sizeof(int),0,(struct sockaddr *)&addrDest,l);

  // Envoie Nom a dest
  sendto(dSEnv,filename,taille,0,(struct sockaddr *)&addrDest,l);
  
  // Envoie Taille Fichier a dest
  sendto(dSEnv,&filesize,sizeof(int),0,(struct sockaddr *)&addrDest,l);

  // Envoie Fichier à dest
  sendto(dSEnv,bufEnvoi,filesize,0,(struct sockaddr *)&addrDest,l);

  // Exit
  free(filename);
  free(filepath);
  free(bufEnvoi);
  fclose(file);

  pthread_exit(0);
}

void *recevoir_t()
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

    if(strcmp(msg,"file") == 0)
    {
      
      printf("Debut reception fichier\n");
      
      // Thread pour recevoir
      pthread_t recevoir_f;
      pthread_create(&recevoir_f,0,recevoir_file,0);
      
    }
    else //message normal
    {
      printf("%s\n",msg);
    }
    
  }
    
}
void *envoyer_t()
{
  //crée la variable pour l'envoi
  char *msg = (char*) malloc(tailleMax * sizeof(char));
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
    else if(strcmp(msg,"file") == 0)
    {
      // demande pseudo (dans msg)
      printf("Entrez le pseudo du destinataire : ");
      fgets(msg,tailleMax,stdin);
      pos = strchr(msg,'\n');
      *pos = '\0';

      // l'envoie au serveur
      send(dS,msg,strlen(msg)+1,0); 

      // reçoit port destinataire UDP
      struct sockaddr_in addrDest;
      int rep = recv(dS, &addrDest, sizeof(struct sockaddr_in),0);
      if (rep < 0)
      { //gestion des erreurs
	perror("Client : Recevoir port : recv -1 ");
      }
      else if (rep == 0)
      {
	perror("Client : Recevoir : recv 0 ");
      }
    
      printf("Addr dest : %s : %d \n", inet_ntoa(addrDest.sin_addr), ntohs(addrDest.sin_port));
      
      if(ntohs(addrDest.sin_port) != 0) // Si 0, arrete, sinon, continue
      {
	// Thread envoi
	pthread_t envoyer_f;
	pthread_create(&envoyer_f,0,envoyer_file,&addrDest);
	pthread_join(envoyer_f,0);

        printf("Fin envoi fichier\n");
      }
      else
      {
	printf("Impossible de joindre le client \n");
      }
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

  int rep;
  
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
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur/serveur
  
  inet_pton(AF_INET,argv[1],&(ad.sin_addr)); //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  socklen_t lgA = sizeof(struct sockaddr_in);
  
  connect(dS,(struct sockaddr *)&ad,lgA);//TODO erreur

  //envoyer le pseudo au serveur
  send(dS,pseudo,strlen(pseudo)+1,0);//TODO erreur

  //creer socket UDP pour rec

  dSRec = socket(PF_INET,SOCK_DGRAM,0); //pas encore bind

  //recevoir notre num de port a bind
  printf("Reception Addresse (ack de conn)\n");
  
  rep = recv( dS, &adUDP, sizeof(struct sockaddr_in), 0); //ce recv ne marche pas ?
  if (rep < 0)
  { //gestion des erreurs
    perror("Client : Recevoir : recv -1 ");
  }
  else if (rep == 0)
  {
    perror("Client : Recevoir : recv 0 ");
  }
  
  printf("fin Rec Addresse\n");
  
  printf("Addr Recue : %s : %d \n", inet_ntoa(adUDP.sin_addr), ntohs(adUDP.sin_port));

  //bind port UDP pour reception
  if (bind(dSRec,(struct sockaddr*)&adUDP,sizeof(adUDP)) < 0)
  {
    perror("bind UDP");
  }
    
  //Communiquer
  printf("Bienvenue dans cette messagerie \n");

  //Separer la lecture et l'ecriture en fonction, puis les appeler en thread
  
  pthread_t tRecevoir;
  pthread_t tEnvoyer;

  pthread_create(&tRecevoir,0,recevoir_t,0);
  pthread_create(&tEnvoyer,0,envoyer_t,0);

  pthread_join(tRecevoir,0);
  printf("fin thread reception : fermer prog\n");
  pthread_join(tEnvoyer,0);
  
  close(dS);

  return 0;
  
}
