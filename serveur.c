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

// port TCP pour general

int dS1;

struct sockaddr_in ad1;

// Mutex pour modif liste client

pthread_mutex_t mu;

// Limites sur taille reception

int tailleMax;

int taillePseudoMax;

int nombreSalon;

// Id incremente a chaque ajout de client

int id;

struct client
{
  int dSC;
  int id;
  struct sockaddr_in aC;
  socklen_t lg;
  struct client *pere;
  struct client *fils;

  int salon;

  char *pseudo;
    
  pthread_t thread;
  
} *ctemp;

struct client** salons; // c est la tete de liste, et correspond à un client pas encore connecte

void efface(struct client *cl)//efface le client
{
  printf("Efface %s : id %d\n", cl->pseudo, cl->id);

  //mutex lock
  pthread_mutex_lock(&mu);
  
  //Enleve de la liste / dechaine et rechaine
  if(cl->fils != NULL)//si on n'est pas la fin de la liste
  {
    cl->fils->pere = cl->pere;
  }
  
  if(cl->pere != NULL)//si on est pas le pere de tout les clients de ce serveur
  {
    cl->pere->fils = cl->fils;
  }
  else // si on est le 1er du serveur
  {
    salons[cl->salon] = cl->fils;
  }

  //mutex unlock
  pthread_mutex_unlock(&mu);
  
  //Close son port
  close(cl->dSC);

  //free
  free(cl->pseudo);
  free(cl);
}

void nettoieSalon(struct client *cl)//efface tout les clients 1 par 1 du salon, a partir de ce client
{
  
  struct client *cSuivant;
  
  while(cl != NULL)
  {
    cSuivant = cl->fils;
    
    efface(cl);

    cl = cSuivant;
  }
  
}

void effaceClients()
{
  for (int i = 0; i < nombreSalon; i++)
  {
    printf("Nettoyage de tout les clients du salon %d\n",i);
    nettoieSalon(salons[i]);
  }

  free(salons);
}

void fin() //FONCTION QUI TRAITE LES CTRL+C
{
  printf("\n Au revoir \n");
  
  // Fermer tout les dsc des clients (liste)
  effaceClients();
  
  // Fermer le serveur
  close(dS1);
  exit(0);
  
}

void insereClientDebutSalon(struct client **c)
{ // insere le client au debut du salon precisé dans client->salon
  // Data : adresse du client a inserer au debut du salon
  // Process : insere le client au debut du salon qu''il a demandé
  // Resultat : ne retourne rien
  
  pthread_mutex_lock(&mu);

  (*c)->fils = salons[(*c)->salon];

  salons[(*c)->salon] = *c;

  if( (*c)->fils != NULL)
  {
    (*c)->fils->pere = (*c);
  }
  
  pthread_mutex_unlock(&mu);

}

void *broadcast(void* cl) // recoit un message d'un client, et le diffuse a tout les autres clients, ce en boucle
{
  struct client *cActif = (struct client*)cl;
  
  int taille;

  char* msg = (char*) malloc((tailleMax+1) * sizeof(char)); // recoit le message
  char* buf = (char*) malloc((tailleMax+1+strlen(cActif->pseudo)+3) * sizeof(char)); 
  
  int fini = 0;

  printf("Debut Thread %s\n",cActif->pseudo);

  // recoit le salon dont le client veut faire partie
  taille = recv(cActif->dSC, &(cActif->salon), sizeof(int), 0);
  if (taille < 0)
  { //gestion des erreurs
    perror("Serveur : Thread : recv -1 ");
    fini = 1;
  }
  else if (taille == 0)
  {
    perror("Serveur : Thread : recv 0 ");
    fini = 1;
  }

  // assigne le client au salon correspondant si valide
  if(cActif->salon >= 0 && cActif->salon < nombreSalon)
  {    
    insereClientDebutSalon(&cActif);
  }
  else //quitte sinon
  {
    printf("%s demande un salon non valide",cActif->pseudo);
    fini = 1;
  }
  
  while(fini == 0)
  {
    taille = recv(cActif->dSC, msg, tailleMax,0);
    printf("recv depuis %s\n",cActif->pseudo);
    
    if (taille < 0)
    { //gestion des erreurs
      perror("Serveur : Thread : recv -1 ");
      fini = 1;
    }
    else if (taille == 0)
    {
      perror("Serveur : Thread : recv 0 ");
      fini = 1;
    }
    else if(strcmp(msg,"file")==0) // file ou mess normal ? ici, file
    {
      // cas file : on attend le pseudo de la cible

      int rep;

      printf("Debut file recv pour %s\n",cActif->pseudo);
      
      taille = recv(cActif->dSC, msg, tailleMax,0); // on attend un pseudo
      if (taille < 0)
      { //gestion des erreurs
	perror("Serveur : Thread : recv -1 ");
      }
      else if (taille == 0)
      {
	perror("Serveur : Thread : recv 0 ");
      }
      else
      {

	// On recherche s'il y a un client avec ce pseudo
	int trouve = 0;
	struct client *cDest = salons[cActif->salon];

	printf("recherche si %s existe\n",msg);
	
	while (cDest != NULL && trouve == 0)
        {
	  
	  if (strcmp(msg,cDest->pseudo)==0 && cDest->id != cActif->id)
	  {
	    trouve = 1;
	  }

	  if(trouve == 0)
	  {
	    cDest = cDest->fils;
	  }
	  
	}//arret while : fin de cDest ou trouve un valide

	// Si pas de valide trouve (existe pas, lui-même, ...), arrete et envoie addr "0" a cActif
	if (trouve == 0)
	{
	  printf("Pseudo pas valide\n");
	  
	  struct sockaddr_in addrDest;
	  addrDest.sin_port = htons(0);
	  
	  rep = send(cActif->dSC, &addrDest, sizeof(struct sockaddr_in), 0);
	  if (rep < 0)//gestion des erreurs
	  {
	    perror("Serveur : thread : send -1 ");
	  }
	  else if (rep == 0)
	  {
	    perror("Serveur : thread : send 0 ");
	  }
	  
	}
	else  
	{ // Si valide, envoie "file" a cDest

	  printf("Pseudo valide, envoie de \"file\" a %s\n",cDest->pseudo);
	  
	  strcpy(msg,"file");
	  
	  rep = send(cDest->dSC, msg, strlen(msg)+1, 0);
	  if (rep < 0)//gestion des erreurs
	  {
	    perror("Serveur : thread : send -1 ");
	  }
	  else if (rep == 0)
	  {
	    perror("Serveur : thread : send 0 ");
	  }	  

	  // Transmet coord cDest a cActif (sockaddr_in)
	  printf("Envoie addr de %s a %s\n",cDest->pseudo, cActif->pseudo);

	  struct sockaddr_in adC = cDest->aC;
	  
	  printf("Addr = %s : %d \n", inet_ntoa(adC.sin_addr), ntohs(adC.sin_port));
	  
	  rep = send( cActif->dSC, &adC, sizeof(struct sockaddr_in), 0);
	  if (rep < 0)//gestion des erreurs
	  {
	    perror("Serveur : thread : send -1 ");
	  }
	  else if (rep == 0)
	  {
	    perror("Serveur : thread : send 0 ");
	  }

	  //fin du role du serveur dans l'envoi de fichier
	  
	}
      }
    }
    else   // reception normale, on envoie aux autres clients
    {
      if (taille == tailleMax) // pour etre sur d'afficher tout le message, et pas depasser
      {
	char *pos = strchr(msg,'\0');
	*pos = ' ';
	msg[tailleMax] = '\0';
      }

      // cree buffer avec pseudo : msg
      strcpy(buf,cActif->pseudo);
      strcat(buf," : ");
      strcat(buf,msg);

      //Pour chaque autre client :

      struct client *cDest = salons[cActif->salon];

      while(cDest != NULL && fini == 0)
      {

	if(cDest->id != cActif->id)
	{
	  //envoie au client receveur
	  
	  printf("send depuis %s vers %s\n",cActif->pseudo, cDest->pseudo);
	  int rep = send(cDest->dSC, buf, strlen(buf)+1, 0);
	  if (rep < 0)//gestion des erreurs
	  {
	    perror("Serveur : thread : send -1 ");
	    fini = 1;
	  }
	  else if (rep == 0)
	  {
	    perror("Serveur : thread : send 0 ");
	    fini = 1;
	  }
	}

	cDest = cDest->fils;

      }
      
    }
    
  }
  
  // Fini : enlever le client de la liste, ferme son port (dans efface()) puis free la memoire
  
  efface(cActif);

  free(msg);

  printf("Fin thread\n");
  
  return 0;
}

struct client *clientVierge()
{
  /*  Crée un nouveau client, avec un id et pas d'adresse
   */

  struct client *c2 = (struct client*) malloc(sizeof(struct client));

  c2->dSC = -1;
  c2->id = id;
  c2->lg = sizeof(struct sockaddr_in);
  c2->salon = 0;

  c2->pere = NULL;
  c2->fils = NULL;

  id++;

  return c2;
  
}

int nombreClients(struct client *client)// compte le nombre de clients dans ce salon
{
  
  if (client != NULL)
  {
    return 1 + nombreClients(client->fils);
  }
  else
  {
    return 0;
  }
  
}

int main(int argc, char* argv[]) // serveur
{
  if(argc < 2)
  {
    printf("Probleme de lancement\nUsage: ./serveur \"nbrSalons\"");
    exit(0);
  }
  
  signal(SIGINT,fin);

  pthread_mutex_init(&mu,NULL);
  
  tailleMax = 2000;

  taillePseudoMax = 50;

  nombreSalon = atoi(argv[1]);

  printf("Initiation des %d salons\n",nombreSalon);

  salons = (struct client**) malloc(nombreSalon * sizeof(struct client *));

  for(int i = 0; i < nombreSalon; i++)
  {
    salons[i] = NULL;
  }

  id = 0;  
  
  // Creer socket TCP
  dS1 = socket(PF_INET,SOCK_STREAM,0); 
  if (dS1 == -1)
  {
    perror("Serveur : socket TCP ");
    raise(SIGINT);
  }
  
  //Nommer le serveur
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
    printf("Serveur : je suis sourd\n");
    raise(SIGINT);
  }

  socklen_t lgS1 = sizeof(struct sockaddr_in); 
  if (getsockname(dS1,(struct sockaddr*)&ad1,&lgS1)<0) // associe un port au socket ?
  {
    printf("Erreur du getsockname \n");
    raise(SIGINT);
  }

  //pour lancer le client avec le bon port
  
  printf("Serveur : mon numero de port est : %d \n",  ntohs(ad1.sin_port));

  int rep;
  
  //Attendre les clients
  
  while(1)
  {
    //crée un nouveau client
    ctemp = clientVierge();
    
    //attendre un nouveau client
    printf("J'attend un client\n");
    ctemp->dSC = accept(dS1, (struct sockaddr*) &(ctemp->aC), &(ctemp->lg)); // accepter la connexion client
    if (ctemp->dSC < 0)
    { //gere les erreurs
      perror("Serveur : Accept ");
      raise(SIGINT);
    }

    //recevoir pseudo
    ctemp->pseudo = (char*) malloc((taillePseudoMax+1) * sizeof(char));
    ctemp->pseudo[taillePseudoMax] = '\0';
    
    int taille = recv(ctemp->dSC, ctemp->pseudo, taillePseudoMax,0);
    if (taille < 0)
    { //gestion des erreurs
      perror("Pseudo : recv -1 ");
    }
    else if (taille == 0)//recu un pseudo vide de la part de ce client
    {
      perror("Pseudo : recv 0 ");     
    }
    else if (taille == 1)//Le pseudo est juste "\0", pas de char
    {
      printf("Ce client est anonyme, il est renommé Anon%d\n",ctemp->id);
      sprintf(ctemp->pseudo,"Anon%d",ctemp->id); 
    }

    //envoie le port du dest au dest (pour ouvrir le bon UDP)
    printf("Envoie addr de %s a lui même\n",ctemp->pseudo );
	  
    printf("Addresse : %s : %d \n", inet_ntoa(ctemp->aC.sin_addr), ntohs(ctemp->aC.sin_port));
	  
    rep = send( ctemp->dSC, &(ctemp->aC), sizeof(struct sockaddr_in), 0); 	  
    if (rep < 0)//gestion des erreurs
    {
      perror("Serveur : thread : send -1 ");
    }
    else if (rep == 0)
    {
      perror("Serveur : thread : send 0 ");
    }

    //envoie le nombre de salons au client
    rep = send( ctemp->dSC, &nombreSalon, sizeof(int), 0); 	  
    if (rep < 0)//gestion des erreurs
    {
      perror("Serveur : thread : send -1 nbrSalon");
    }
    else if (rep == 0)
    {
      perror("Serveur : thread : send 0 nbrSalon");
    }

    //pour chaque salon, envoi le nombre de clients(facultatif)
    int cmp = 0;
    for(int i = 0; i < nombreSalon; i++)
    {
      cmp = nombreClients(salons[i]);
      
      rep = send( ctemp->dSC, &cmp, sizeof(int), 0); 	  
      if (rep < 0)//gestion des erreurs
      {
	perror("Serveur : thread : send -1 nbrSalon");
      }
      else if (rep == 0)
      {
	perror("Serveur : thread : send 0 nbrSalon");
      }
      
    }

    //lance dans thread avec ce client
    
    taille = pthread_create(&(ctemp->thread),0,&broadcast,(void*)ctemp);
    if (taille < 0)
    {
      perror("Creation Thread Client actuel ");
    }
    
  }

}
