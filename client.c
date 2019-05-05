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

int dSRec; //descripteur pour reception (en UDP) => besoin de bind, port donné par serv

struct sockaddr_in adUDP;

int tailleMax; // taille max des messages pour fgets

char *pseudo;

//buffer pour envoi

char* filepath;

char* filename;

char* bufEnvoi;

int filesize;

int choixFichier()
{
  // lister les fichiers disponibles
  DIR *dp;
  struct dirent *ep;
  
  filepath = (char*) malloc(tailleMax * sizeof(char));
  strcpy(filepath,"./fileSource/");

  filename = (char*) malloc(tailleMax * sizeof(char));  
  
  dp = opendir(filepath);
  
  if (dp != NULL)
  {
    printf("Voilà la liste de fichiers :\n");
    ep = readdir(dp);
    while(ep)
    {
      if(strcmp(ep->d_name,".")!=0 && strcmp(ep->d_name,"..")!=0)
      {
	printf("%s\n",ep->d_name);
      }
      ep = readdir (dp);
    }    
    (void) closedir (dp);
  }
  else
  {
    perror ("opendir : ./fileSource/ n'existe pas");

    return -1;
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

    return -1;
  }

  // repere la taille du fichier
  struct stat res;
  int ret = stat(filepath,&res);
  if (ret < 0)
  {
    perror("File sendd stat : ");
    return -1;
  }
  filesize = res.st_size;

  bufEnvoi = (char*) malloc(filesize * sizeof(char));
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

  //fermer file
  fclose(file);

  return 0;

}

void *recevoir_file()
{
  printf("Debut reception file\n");

  // Fait addresse expediteur
  struct sockaddr_in adExp;
  socklen_t len = sizeof(struct sockaddr_in);

  int rep;
  
  // Recoit Taille Nom
  int taille;
  rep = recvfrom(dSRec,&taille,sizeof(int),0,(struct sockaddr*)&adExp,&len);
  if (rep < 0)
  { //gestion des erreurs
    perror("TailleNom : recvfrom -1 ");
  }
  else if (rep == 0)
  {
    perror("Taillenom : recvfrom 0 ");
  }

  char *filenameRec = (char *) malloc((taille+1) * sizeof(char));

  // Recoit Nom Fichier
  rep = recvfrom(dSRec,filenameRec,taille,0,(struct sockaddr*)&adExp,&len);
  if (rep < 0)
  { //gestion des erreurs
    perror("Nom : recvfrom -1 ");
  }
  else if (rep == 0)
  {
    perror("Nom : recvfrom 0 ");
  }

  // Ouvre le fichier pour ecrire
  char *filepathRec = (char*)malloc(tailleMax * sizeof(char));
  strcpy(filepathRec,"./fileDest/");
  strcat(filepathRec,filenameRec);
  FILE *file = fopen(filepathRec,"wb");

  // Recoit Taille fichier
  int filesizeRec;
  rep = recvfrom(dSRec,&filesizeRec,sizeof(int),0,(struct sockaddr*)&adExp,&len);
  if (rep < 0)
  { //gestion des erreurs
    perror("Taille Fichier : recvfrom -1 ");
  }
  else if (rep == 0)
  {
    perror("Taille Fichier : recvfrom 0 ");
  }

  char * bufRec = (char*) malloc(filesizeRec * sizeof(char));

  // Recoit fichier (et ecrit ?)
  rep = recvfrom(dSRec,bufRec,filesizeRec,0,(struct sockaddr*)&adExp,&len);
  if (rep < 0)
  { //gestion des erreurs
    perror("Fichier : recvfrom -1 ");
  }
  else if (rep == 0)
  {
    perror("Fichier : recvfrom 0 ");
  }
  
  printf("Reception de %s, enregistre dans %s, de taille %d\n",filenameRec,filepathRec,filesizeRec);

  //ecrire fichier
  fwrite(bufRec,filesizeRec,1,file);
  
  // Exit thread
  free(bufRec);
  free(filepathRec);
  free(filenameRec);
  fclose(file);
  
  printf("Fin reception file\n");

  pthread_exit(0);
  
}

void *envoyer_file(void * adrDest)
{
  int rep;
  
  //recuperer addr et allouer place pour filename et filepath
  struct sockaddr_in addrDest = *((struct sockaddr_in *) adrDest);
  
  // Cree Socket UDP pour envoi (pas le meme que celui bind pour recevoir)
  int dSEnv = socket(PF_INET,SOCK_DGRAM,0);
  if (dSEnv == -1)
  {
    perror("Socket UDP ");
  }
  
  socklen_t l = sizeof(struct sockaddr_in);  

  // Envoie Taille Nom a dest
  int tailleNom = (strlen(filename)+1);
  
  rep = sendto(dSEnv,&tailleNom,sizeof(int),0,(struct sockaddr *)&addrDest,l);
  if (rep < 0)//gestion des erreurs
  {
    perror("Send Taille nom -1 ");
  }
  else if (rep == 0)
  {
    perror("Send Taille nom 0 ");
  }

  // Envoie Nom a dest
  rep = sendto(dSEnv,filename,tailleNom,0,(struct sockaddr *)&addrDest,l);
  if (rep < 0)//gestion des erreurs
  {
    perror("Send nom -1 ");
  }
  else if (rep == 0)
  {
    perror("Send nom 0 ");
  }
  
  // Envoie Taille Fichier a dest
  rep = sendto(dSEnv,&filesize,sizeof(int),0,(struct sockaddr *)&addrDest,l);
  if (rep < 0)//gestion des erreurs
  {
    perror("Send Taille -1 ");
  }
  else if (rep == 0)
  {
    perror("Send Taille 0 ");
  }

  // Envoie Fichier à dest
  rep = sendto(dSEnv,bufEnvoi,filesize,0,(struct sockaddr *)&addrDest,l);
  if (rep < 0)//gestion des erreurs
  {
    perror("Send Fichier -1 ");
  }
  else if (rep == 0)
  {
    perror("Send Fichier 0 ");
  }

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
      
      // Thread pour recevoir
      pthread_t recevoir_f;
      rep = pthread_create(&recevoir_f,0,recevoir_file,0);
      if (rep < 0)
      {
	perror("Creation Thread Reception file ");
      }
      pthread_join(recevoir_f,0);
      
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
    int rep = send(dS,msg,strlen(msg)+1,0); // +1 pour envoyer le \0 
    if (rep < 0)//gestion des erreurs
    {
      perror("Send -1 ");
    }
    else if (rep == 0)
    {
      perror("Send 0 ");
    }
    
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
      rep = send(dS,msg,strlen(msg)+1,0);
      if (rep < 0)//gestion des erreurs
      {
	perror("Send pseudo file -1 ");
      }
      else if (rep == 0)
      {
	perror("Send pseudo file 0 ");
      }

      // reçoit port destinataire UDP
      struct sockaddr_in addrDest;
      rep = recv(dS, &addrDest, sizeof(struct sockaddr_in),0);//autre recv devient prioritaire
      if (rep < 0)
      { //gestion des erreurs
	perror("Client : Recevoir port : recv -1 ");
      }
      else if (rep == 0)
      {
	perror("Client : Recevoir port : recv 0 ");
      }
    
      printf("Addr dest : %s : %d \n", inet_ntoa(addrDest.sin_addr), ntohs(addrDest.sin_port));
      
      if(ntohs(addrDest.sin_port) != 0) // Si 0, arrete, sinon, continue
      {
	//choisir fichier : cette fonction copie le fichier dans bufEnvoi si reussite
	rep = choixFichier();
	if (rep < 0)
	{
	  bufEnvoi = (char*)malloc( sizeof(char));
	  bufEnvoi[0] = '\0';
	}

	pthread_t envoyer_f;
	rep = pthread_create(&envoyer_f,0,envoyer_file,&addrDest);
	if (rep < 0)
	{
	  perror("Creation Thread envoi file ");
	}
	
	pthread_join(envoyer_f,0);
	
        printf("Fin envoi fichier\n");
      }
      else
      {
	printf("Impossible de joindre le client \n");
      }
	
      // Exit : nettoyer buf envoi car alloué car choixFichier()
      free(bufEnvoi);		
      free(filename);
      free(filepath);
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
  if (dS == -1)
  {
    perror("Socket TCP ");
    exit(0);
  }

  tailleMax = 2000;
  
  
  //Demander une connexion au serveur
  ad.sin_family = AF_INET;
  ad.sin_port = htons(atoi(argv[2])); // port du receveur/serveur, pas d'erreur detectables sur htons
  
  //associe l'adresse ip correspondant à la chaine de char, et la mets dans ad.sin_addr
  rep = inet_pton(AF_INET,argv[1],&(ad.sin_addr));
  if (rep <0)
  {
    perror("inet_pton : Famille d'adresse pas valide ");
    exit(0);
  }
  else if (rep == 0)
  {
    perror("inet_pton : Adresse pas valide ");
    exit(0);
  }
  
  socklen_t lgA = sizeof(struct sockaddr_in);
  
  rep = connect(dS,(struct sockaddr *)&ad,lgA);
  if (rep < 0)
  {
    perror("Connect ");
    exit(0);
  }

  //envoyer le pseudo au serveur
  rep = send(dS,pseudo,strlen(pseudo)+1,0);
  if (rep < 0)//gestion des erreurs
  {
    perror("Send pseudo init -1 ");
    exit(0);
  }
  else if (rep == 0)
  {
    perror("Send pseudo init 0 ");
    exit(0);
  }

  //creer socket UDP pour rec

  dSRec = socket(PF_INET,SOCK_DGRAM,0); //pour la reception : il faut le bind (cf ci-apres)
  if (dSRec == -1)
  {
    perror("Socket UDP ");
    exit(0);
  }

  //recevoir notre num de port a bind
  
  rep = recv( dS, &adUDP, sizeof(struct sockaddr_in), 0); 
  if (rep < 0)
  { //gestion des erreurs
    perror("Recevoir Adresse : recv -1 ");
    exit(0);
  }
  else if (rep == 0)
  {
    perror("Recevoir Adresse : recv 0 ");
    exit(0);
  }
  
  printf("Addr Recue : %s : %d \n", inet_ntoa(adUDP.sin_addr), ntohs(adUDP.sin_port));

  //bind port UDP pour reception
  if (bind(dSRec,(struct sockaddr*)&adUDP,sizeof(adUDP)) < 0)
  {
    perror("bind UDP ");
    exit(0);
  }
    
  //Communiquer
  printf("\n\n-------------------Bienvenue dans cette messagerie-------------------\n\n");

  //Separer la lecture et l'ecriture en fonction, puis les appeler en thread
  
  pthread_t tRecevoir;
  pthread_t tEnvoyer;

  rep = pthread_create(&tRecevoir,0,recevoir_t,0);
  if (rep < 0)
  {
    perror("Creation Thread Reception ");
    exit(0);
  }
  rep = pthread_create(&tEnvoyer,0,envoyer_t,0);
  if (rep < 0)
  {
    perror("Creation Thread envoi ");
    exit(0);
  }

  pthread_join(tRecevoir,0);
  pthread_join(tEnvoyer,0);

  printf("Ce message n'est pas censé s'afficher\n");
  
  close(dS);

  return -1;
  
}
