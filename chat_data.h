#ifndef CHAT_DATA_H
#define CHAT_DATA_H

#include "architecture.h"
#include <stdio.h>
#include <stdlib.h>

// Mot magique pour vérifier l'intégrité des données dans l'EEPROM
// Il faut modifier cette valeur si l'ont change la structure de données ou
// les paramètres ci-dessous, pour éviter d'avoir une mémoire corrompue.
#define MAGIC_WORD (0xABCD)

// Taille FIXE pour les noms d'utilisateur ou groupes (4 char)
// ATTENTION : il n'y a pas de marqueur de fin '\0' !!
#define NAME_SIZE (4)

// Nombre maximum de favoris, contacts et groupes
#define MAX_FAVORITES (50)
#define MAX_CONTACTS (200)
#define MAX_GROUPS (100)

// Structure représentant un contact de chat
struct chat_contact {
  // Il a un nom (username or group name)
  char name[NAME_SIZE];

  // Et un compteur dont on garde la trace du dernier message vu de ce contact
  // pour éviter les doublons de messages
  uint64_t last_seen_counter;
};

// Structure représentant les données à stocker dans l'EEPROM
// Il y a deux parties : les paramètres de configuration du module LoRa
// et les données de l'application (chat).
struct chat_data {
  uint16_t magic_word;

  // Paramètres de configuration du module LoRa
  uint32_t lora_channel;
  uint8_t lora_bw;
  uint8_t lora_sf;
  uint8_t lora_cr;
  uint8_t lora_crc;
  uint8_t lora_implicit;
  uint8_t lora_syncword;

  // Données de l'utilisateur local (son nom et son compteur de messages)
  struct chat_contact local_user;

  // Données de l'application : les favoris, les contacts et les groupes de chat
  struct chat_contact chat_favorites[MAX_FAVORITES];
  struct chat_contact chat_contacts[MAX_CONTACTS];
  struct chat_contact chat_groups[MAX_GROUPS];
};

// Comparaison de deux noms de contact (4 char)
static inline int name_cmp(const char a[NAME_SIZE], const char b[NAME_SIZE]) {
  return (a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2] &&
         (a)[3] == (b)[3];
}

// Permet de copier un nom de contact (4 char)
static inline void name_cpy(char dest[NAME_SIZE], const char src[NAME_SIZE]) {
  (dest)[0] = (src)[0];
  (dest)[1] = (src)[1];
  (dest)[2] = (src)[2];
  (dest)[3] = (src)[3];
}

// Permet l'affichage d'un tableau de contacts de chat (favoris, contacts ou
// groupes) de manière lisible (pour le debug)
static inline void print_contact_table(size_t size,
                                       struct chat_contact c[size]) {
  for (size_t i = 0; i < size; i++) {
    if (c[i].name[0] != '\0') {
      printf("  Name=%.4s, last_seen_counter=%lu\n", c[i].name,
             (unsigned long)c[i].last_seen_counter);
    }
  }
}

#endif /* CHAT_DATA_H */
