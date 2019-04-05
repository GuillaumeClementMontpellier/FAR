# FAR
Projet FAR : messagerie instantanee

## Iteration 1:

Un client essaie de se connecter a un serveur, puis il passe dans une boucle : il attend/recoit un message, puis l'utilisateur saisie une chaine de char, le client envoie cette chaine de char, et recommence la boucle. La maniere de sortir de cette boucle est de recevoir ou d'entrer 'fin'.

Le serveur commence a se mettre en attente de connexion : il attend 2 clients.

Quand le premier client se connecte, le serveur lui envoie un message "Vous etes le premier client", puis attend le second.

L'envoi de ce message permet que le premier client, en le recevant, passe à l'etape suivante de la boucle (envoyer un message).

Ensuite, une fois que le second client se connecte, il entre dans une boucle, qui consiste :
 - attendre un message du client 1
 - le transmettre au client 2
 - attendre un message du client 2
 - le transmettre au client 1
 - recommence

Cette boucle s'arrete quand un des clients se deconnecte (ce qui est reperé quand recv renvoie 0), alors le serveur ferme les ports des clients, et se remet à attendre 2 clients.

Lorsque un client envoie 'fin', ce client se deconnecte juste apres. Le serveur relai juste le message sans se soucier du contenu, donc l'autre client recoit 'fin'. Il se deconnecte donc aussi sans rien renvoyer, donc le recv du serveur renvoie bien 0, ce qui cause la sortie de la boucle d'envoi receptoin, et donc l'attente de 2 clients.

Le serveur intercepte le signal ctrl+c (SIGINT) afin de fermer les sockets aavant de se terminer.

## Iteration 2:

Le serveur et le client ont ete changé pour respecter l'intitulé.

Pour le client, l'envoie et la reception ont ete mise dans des fonctions pour pouvoir etre utilise en thread.

Pour le serveur, la transmission dans chaque dend ont été mise dans des fonctions pour pouvoir utiliser des thread.

Les clients s'arrete en lançant un signal SIGUSR1, qui lance une fonction qui ferme les sockets puis arrete le programme.

