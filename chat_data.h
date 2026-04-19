#ifndef CHAT_DATA_H
#define CHAT_DATA_H

#include "architecture.h"
#include "mutex.h"
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
#define MAX_CONTACTS (100)
#define MAX_GROUPS (50)

// Structure représentant un contact de chat
struct chat_contact {
  // Il a un nom (username or group name)
  char name[NAME_SIZE];

  // Et un compteur dont on garde la trace du dernier message vu de ce contact
  // pour éviter les doublons de messages
  uint32_t last_seen_counter;

  // Permet de différencier les favoris des contacts normaux
  uint8_t is_favorite;
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

  // Données de l'application : les contacts et les groupes de chat
  struct chat_contact chat_contacts[MAX_CONTACTS];
  char chat_groups[MAX_GROUPS][NAME_SIZE];
};

// Structure partagée des données du chat, protégée par un mutex
struct sync_chat_data {
  struct chat_data *chat_data;
  mutex_t *mutex;
};

/* ========================================================================== */

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

/* ========================================================================== */

// Obtenir l'index d'un contact à partir de son nom, ou -1 si le contact
// n'existe pas
static inline int get_contact_index(struct chat_contact *data,
                                    const char name[NAME_SIZE]) {
  for (size_t i = 0; i < MAX_CONTACTS; i++)
    if (name_cmp(data[i].name, name))
      return i;

  return -1;
}

// Obtenir l'index d'un contact vide dans la liste des contacts, ou -1 si la
// liste est pleine
static inline int get_contact_empty_index(struct chat_contact *data) {
  for (size_t i = 0; i < MAX_CONTACTS; i++)
    if (data[i].name[0] == '\0')
      return i;

  return -1;
}

// Obtenir l'index d'un groupe à partir de son nom, ou -1 si le groupe n'existe
// pas
static inline int get_group_index(char group[MAX_GROUPS][NAME_SIZE],
                                  const char name[NAME_SIZE]) {
  for (size_t i = 0; i < MAX_GROUPS; i++)
    if (name_cmp(group[i], name))
      return i;

  return -1;
}

// Obtenir l'index d'un groupe vide dans la liste des groupes, ou -1 si la liste
// est pleine
static inline int get_group_empty_index(char group[MAX_GROUPS][NAME_SIZE]) {
  for (size_t i = 0; i < MAX_GROUPS; i++)
    if (group[i][0] == '\0')
      return i;

  return -1;
}

/* ========================================================================== */

// Permet l'affichage d'un tableau de contacts de chat (favoris, contacts ou
// groupes) de manière lisible (pour le debug)
static inline void print_contact_table(size_t size,
                                       struct chat_contact c[size]) {
  for (size_t i = 0; i < size; i++)
    if (c[i].name[0] != '\0')
      printf("  Name:%.4s, last_seen_counter:%lu, favorite:%u\n", c[i].name,
             c[i].last_seen_counter, c[i].is_favorite);
}

// Permet l'affichage d'un tableau de groupes de chat de manière lisible (pour
// le debug)
static inline void print_group_table(size_t size, char c[size][NAME_SIZE]) {
  for (size_t i = 0; i < size; i++)
    if (c[i][0] != '\0')
      printf("  %.4s", c[i]);

  puts("");
}

// Affichage de l'ensemble des données du chat (pour le debug)
static inline void print_chat_data(struct chat_data *data) {
  printf("LoRa channel: %lu\n", data->lora_channel);
  printf("LoRa bandwidth: %u\n", data->lora_bw);
  printf("LoRa spreading factor: %u\n", data->lora_sf);
  printf("LoRa coding rate: %u\n", data->lora_cr);
  printf("LoRa CRC enabled: %u\n", data->lora_crc);
  printf("LoRa implicit header: %u\n", data->lora_implicit);
  printf("LoRa syncword: %u\n", data->lora_syncword);

  printf("Local user name: %.4s\n", data->local_user.name);
  printf("Local user msg counter: %lu\n", data->local_user.last_seen_counter);

  printf("Chat contacts:\n");
  print_contact_table(MAX_CONTACTS, data->chat_contacts);

  printf("Chat groups:\n");
  print_group_table(MAX_GROUPS, data->chat_groups);
}

#endif /* CHAT_DATA_H */
