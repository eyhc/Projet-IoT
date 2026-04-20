# Projet-IoT : Lora Disaster Chat

Auteurs: Elie Carrot, Arthur Palma

## Cloner le projet

```bash
git clone
git submodule update --init --recursive
```

## Compiler et flasher le projet

```bash
make -j 8 flash
```

## Fonctionnalités

**LoraChat**

- [X] Initialisation automatique du module LoRa
- [X] Initialisation automatique du chat (eeprom ou valeurs par défaut)
- [X] Configuration du nom d'utilisateur
- [X] Sauvegarde périodique des données du système dans l'eeprom 
     (toutes les 10 minutes)

- [X] Ajouter un favori
- [X] Lister les contacts
- [X] Supprimer un favori (le laisse dans la liste des contacts).

- [X] Inscription à un groupe de discussion
- [X] Affichage de la liste des groupes de discussion auxquels je suis inscrit
- [X] Suppression de mon inscription à un groupe de discussion

- [X] Envoi de message à un contact (enregistré ou non / pas d'inscription auto)
- [X] Envoi de message sur un groupe de discussion (inscription automatique)
- [X] Affichage du message émis complet (exp, dest, contenu, etc.)

- [X] Filtrage des messages reçus (affichage uniquement des messages destinés à 
      moi ou à un groupe de discussion auquel je suis inscrit)
- [ ] Sauvegarder et lister les derniers messages reçus (filtrés ou non)

- [X] Ajout automatique d'un expéditeur inconnu à la liste des contacts
- [ ] Eviction LRU des contacts si la liste est pleine.

- [X] Emission télémétrique à une fréquence régulière paramétrable.

**LoraMeshChat**

- [X] Ignorer les messages déjà reçus (via compteur de message expéditeur).
- [ ] Queue de messages en attente de réémission.
- [ ] Réémettre un message (réseau maillé) si présence du ttl dans le message.
- [X] Gestion du ttl dans les messages (sending & parsing).
- [X] Configurer le seuil SNR pour la réémission d'un message (pour éviter les 
      réémissions simultanées de noeuds proches l'un de l'autre).

Politique de réémission : 

1. un message est réémis si le compteur de réémission du message est supérieur
   à 0 après un temps d'attente aléatoire entre 1 et 10 secondes.
2. le compteur de réémission du message est décrémenté à chaque réémission.
3. réémission ignorée si le message a déjà été réémis avec un rapport signal sur
   bruit supérieur à 10 dB (pour éviter les réémissions simultanées de noeuds 
   proches l'un de l'autre).

## Commandes disponibles (lora module exclu)

```
eeprom_print : Affichage de la sauvegarde du chat dans l'eeprom
info : Affichage des données du chat
username get : Affichage du nom d'utilisateur
username set <username> : Modification du nom d'utilisateur (4 caractères ascii)
favorite add <contact> : Ajout d'un contact à la liste des favoris
favorite ls : Affichage de la liste des favoris
favorite rm <contact> : Suppression d'un contact de la liste des favoris
group add <group> : Inscription à un groupe de discussion
group ls : Affichage de la liste des groupes de discussion auxquels je suis inscrit
group rm <group> : Suppression de mon inscription à un groupe de discussion
contact ls : Affichage de la liste des contacts
send_broadcast <message> : Envoi d'un message à tous les utilisateurs
send_contact alias <contact> <message> : Envoi d'un message via son alias 
                                         (4 caractères ascii)
send_contact id    <id> <message> : Envoi d'un message à un contact 
                                    (id dans la liste des contacts)
send_group   alias <group> <message> : Envoi d'un message sur un groupe de discussion
send_group   id    <id> <message> : Envoi d'un message sur un groupe de discussion 
                                    (id dans la liste des groupes de discussion)
force_rcv <message> : Simule la réception d'un message (pour les tests)
force_rcv <message> <rssi> <snr> <toa> : Idem que précedent mais avec des 
                                        paramètres de réception personnalisés
telemetry start : Démarrage de l'émission télémétrique
telemetry stop : Arrêt de l'émission télémétrique
telemetry print : Affichage des données de télémétrie actuelles
telemetry print_lpp_cayenne : Affichage des données de télémétrie actuelles au 
                              format Cayenne LPP
telemetry set period <interval> : Modification de la période d'émission 
                                  télémétrique à interval secondes
telemetry set broadcast <0|1> : Activation ou désactivation de la diffusion de 
                                la télémétrie à tous les utilisateurs.
telemetry set group <group> : Configuration du groupe de discussion destinataire 
                              de la télémétrie.
mesh set snr_threshold <threshold> : Configuration du seuil de rapport signal sur
                                     bruit pour la réémission d'un message (en dB)
mesh set ttl <ttl> : Configuration du ttl maximum d'un message émis ou réémis.
mesh print : Affichage de la queue de messages en attente de réémission.
mesh enable : Activation du mode maillé (réémission des messages avec ttl)
mesh disable : Désactivation du mode maillé.
```

## Fichiers

- `main.c` : point d'entrée du programme, initialisation des modules et lancement du shell.
- `eeprom.h/c` : gestion de la mémoire eeprom pour la sauvegarde des données du chat.
- `lora.h/c` : interface de communication avec le module LoRa (sx127x).
- `message.h/c` : définition de la structure de message et fonctions associées (formatage, envoi, parsing, queue fifo, etc.).
- `chat_data.h`: définition de la structure de données du chat (contacts, groupes, etc.) et fonctions associées.
- `chat.c` : implémentation des commandes du chat et de la logique de gestion des messages, contacts, groupes, etc.
- `group.c` : implémentation des fonctions de gestion des groupes de discussion.
- `contact.c` : implémentation des fonctions de gestion des contacts.
- `telemetry.c` : implémentation de l'émission télémétrique périodique.
- `mesh.c` : implémentation de la logique de réseau maillé (réémission des messages avec ttl).
