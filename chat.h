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

// Lire/Modifier le nom d'utilisateur du chat
// Usage: username get | username set <username> (username = 4 char ascii)
int chat_username(int argc, char *argv[argc]);

#endif /* CHAT_H */
