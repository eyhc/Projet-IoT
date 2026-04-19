/*
 * Ce fichier définit l'ensemble des commandes et données
 *
 * Auteur : ecarrot
 */
#ifndef CHAT_H
#define CHAT_H

#include "architecture.h"
#include "chat_data.h"
#include "mutex.h"

// Structure partagée des données du chat, protégée par un mutex
struct sync_chat_data {
  struct chat_data *chat_data;
  mutex_t *mutex;
};

/* ---- FONCTIONS DU CHAT ---- */

// Initialisation du chat : lecture des données dans l'EEPROM, initialisation du
// module LoRa et lancement du thread de sauvegarde périodique des données dans
// l'EEPROM
// period_s : période de sauvegarde des données dans l'EEPROM, en secondes
void chat_init(uint32_t period_s);

// Affichage d'informations sur le système (nom d'utilisateur, liste contacts,
// etc.)
int chat_info(int argc, char *argv[argc]);

// Lire/Modifier le nom d'utilisateur du chat
// Usage: username get | username set <username> (username = 4 char ascii)
int chat_username(int argc, char *argv[argc]);

// Gestion des favoris
// Usage:
//  - favorite add <contact_name> : ajoute ou passe un contact en favori
//  - favorite rm <contact_name> : retire un contact des favoris
//  - favorite ls : affiche la liste des contacts favoris
int chat_favorite(int argc, char *argv[argc]);

// Gestion des groupes de discussion
// Usage:
//  - group add <group_name> : rejoint un groupe de discussion
//  - group rm <group_name> : quitte un groupe de discussion
//  - group ls : affiche la liste des groupes de discussion
int chat_group(int argc, char *argv[argc]);

// Affichage de la liste des contacts
// Usage: contact ls
int chat_contact(int argc, char *argv[argc]);

#endif /* CHAT_H */
