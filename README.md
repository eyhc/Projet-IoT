# Projet-IoT : Lora Disaster Chat

# Cloner le projet

```bash
git clone
git submodule update --init --recursive
```

# Compiler et flasher le projet

```bash
make -j 8 flash
```

# Fonctionnalités

- [X] Initialisation automatique du module LoRa
- [X] Initialisation automatique du chat (eeprom ou valeurs par défaut)
- [X] Configuration du nom d'utilisateur
- [X] Sauvegarde périodique des données du système dans l'eeprom 
     (toutes les 10 minutes)
- [ ] Ajouter un favori
- [ ] Lister les favoris
- [ ] Supprimer un favori
- [ ] Inscription à un groupe de discussion
- [ ] Affichage de la liste des groupes de discussion auxquels je suis inscrit
- [ ] Suppression de mon inscription à un groupe de discussion
- [ ] Filtrage des messages reçus (affichage uniquement des messages destinés à 
      moi ou à un groupe de discussion auquel je suis inscrit)
- [ ] Envoi de message à un contact favori
- [ ] Envoi de message à un contact non favori (ajout automatique à la liste des 
      contacts)
- [ ] Envoi de message à un groupe de discussion
- [ ] Ajout automatique d'un expéditeur inconnu à la liste des contacts 
      (éviction LRU)
- [ ] Emission télémétrique à une fréquence régulière paramétrable.
- [ ] Ignorer les messages déjà reçus (via compteur de message expéditeur)
- [ ] Réémettre un message (réseau maillé).


Politique de réémission : 

1. A définir
