/*
 * Ce fichier définit les données du système qui seront stockées dans l'EEPROM,
 * ainsi que les fonctions pour lire et écrire ces données.
 *
 * Auteur : apalma
 */
#ifndef EEPROM_H
#define EEPROM_H

#include "architecture.h"
#include "mutex.h"

// Mot magique pour vérifier l'intégrité des données dans l'EEPROM
// Il faut modifier cette valeur si l'ont change la structure de données ou
// les paramètres ci-dessous, pour éviter d'avoir une mémoire corrompue.
#define EEPROM_MAGIC_WORD (0xABCE)

// Taille FIXE pour les noms d'utilisateur ou groupes (4 char)
// ATTENTION : il n'y a pas de marqueur de fin '\0' !!
#define NAME_SIZE (4)

// Nombre maximum de favoris, contacts et groupes
#define MAX_FAVORITES (50)
#define MAX_CONTACTS (200)
#define MAX_GROUPS (100)

// Structure représentant un contact de chat
struct chat_contact {
  // Il a un nom
  char name[NAME_SIZE];

  // Et un compteur dont on garde la trace du dernier message vu de ce contact
  // pour éviter les doublons de messages
  uint64_t last_seen_counter;
};

// Structure représentant les données à stocker dans l'EEPROM
// Il y a deux parties : les paramètres de configuration du module LoRa
// et les données de l'application (chat).
struct eeprom_data {
  uint16_t magic_word;

  // Paramètres de configuration du module LoRa
  uint16_t lora_channel;
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

struct shared_eeprom_data {
  struct eeprom_data *data;
  mutex_t *mutex;
};

// On rend la structure de données de l'EEPROM accessible à tout le système via
// une variable globale, protégée par un mutex pour éviter les problèmes de
// concurrence lors de l'accès à ces données.
// Les fonctions d'accès à l'EEPROM utilisent cette variable globale
extern struct shared_eeprom_data shared_data;

// Initialise la structure de données de l'EEPROM avec les valeurs en mémoire,
// ou avec des valeurs par défaut si l'EEPROM n'était pas initialisée
// lance un thread de sauvegarde périodique de la structure de données dans
// l'EEPROM
void init_eeprom(uint32_t period_sec);

// Fonctions pour lire et écrire les données de l'EEPROM
// Ces fonctions utilisent la variable globale shared_data.
void eeprom_write_data(void);
void eeprom_read_data(void);

// Fonction à utiliser pour s'assurer que la structure de données tient bien
// dans l'EEPROM. Si ce n'est pas le cas, il faut réduire la taille des tableaux
// de contacts ou des groupes
void assert_eeprom_enough_space(void);

// Permet l'affichage d'un tableau de contacts de chat (favoris, contacts ou
// groupes) de manière lisible (pour le debug)
void print_contact_table(size_t size, struct chat_contact c[size]);

// Lit les données de l'EEPROM au format struct eeprom_data,
// et affiche les données lues à l'écran de manière lisible (pour le debug)
int eeprom_print_data_cmd(int argc, char **argv);

#endif /* EEPROM_H */
