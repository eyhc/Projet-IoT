#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include "chat_data.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

// Taille maximale du contenu d'un message (en caractères)
#define MAX_MESSAGE_SIZE 128

enum message_dest_type { DEST_BROADCAST, DEST_CONTACT, DEST_GROUP };

// Structure d'un message
struct message {
  char sender[NAME_SIZE];
  enum message_dest_type dest_type;
  char dest[NAME_SIZE];
  char content[MAX_MESSAGE_SIZE + 1];
  uint32_t counter;
  int32_t ttl; // -1 pour réseau non maillé
};

// Message reçu, avec les métadonnées de réception (RSSI, SNR, ToA)
struct received_message {
  struct message msg;
  int16_t rssi;
  int8_t snr;
  uint32_t toa;
};

/* ==== MESSAGE TO STRING ==== */

// Convertit un message en chaîne de caractères pour expédition.
void sprint_message(size_t buffer_size, char buffer[buffer_size],
                    const struct message *msg);

// Fonction d'affichage d'un message au format éxpédié.
void print_message(const struct message *msg);

/* ==== MESSAGE_LISTENING ==== */

// Convertit un message reçu en données
// 0 - Success, other - Error (invalid format)
int parse_message(size_t buffer_size, char buffer[buffer_size],
                  struct message *msg);

// Détermine si le message m'est destiné à moi ou à un groupe auquel je suis
// inscrit. Retourne 1 si et seulement si le message doit être affiché, 0 sinon
int filter_message(const struct message *msg,
                   const struct sync_chat_data *data);

// Ajoute un message à l'historique de mes messages (messages que j'ai envoyés
// ou reçus destinés à moi)
void add_message_to_my_history(struct message *msg,
                               struct sync_chat_data *data);

// Affiche les derniers messages que j'ai envoyés ou reçus destinés à moi
void print_my_messages(size_t nb_msg);

// Ajoute un message à l'historique des messages destinés à une autre
// destination (différente de moi ou des groupes auxquels je suis inscrit)
void add_message_to_other_dest_history(struct message *msg,
                                       struct sync_chat_data *data);

// Affiche les derniers messages destinés à une autre destination (différente de
// moi ou des groupes auxquels je suis inscrit)
void print_other_dest_messages(size_t nb_msg);

#endif /* MESSAGE_SENDER_H */
