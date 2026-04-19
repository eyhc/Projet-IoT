# Projet-IoT : Lora Disaster Chat

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

- [ ] Envoi de message à un contact (enregistré ou non)
- [ ] Envoi de message à un groupe de discussion
- [ ] Affichage du message émis complet (exp, dest, contenu, etc.)

- [ ] Filtrage des messages reçus (affichage uniquement des messages destinés à 
      moi ou à un groupe de discussion auquel je suis inscrit)
- [ ] Lister les derniers messages reçus.
- [ ] Ajout automatique d'un expéditeur inconnu à la liste des contacts
- [ ] Eviction LRU des contacts si la liste est pleine.

- [ ] Emission télémétrique à une fréquence régulière paramétrable.

**LoraMeshChat**

- [ ] Ignorer les messages déjà reçus (via compteur de message expéditeur)
- [ ] Réémettre un message (réseau maillé) si présence du ttl dans le message.


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
```
