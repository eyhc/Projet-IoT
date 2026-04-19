#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include "chat_data.h"

// Taille maximale du contenu d'un message (en caractères)
#define MAX_MESSAGE_SIZE 128

enum message_dest_type { DEST_BROADCAST, DEST_CONTACT, DEST_GROUP };

// Structure d'un message
struct message {
  char sender[NAME_SIZE];
  enum message_dest_type dest_type;
  char dest[NAME_SIZE];
  char *content;
  uint32_t counter;
};

// Convertit un message en chaîne de caractères pour expédition.
void sprint_message(size_t buffer_size, char buffer[buffer_size],
                    const struct message *msg);

// Fonction d'affichage d'un message au format éxpédié.
void print_message(const struct message *msg);

// Envoie un message via le module LoRa.
int send_message(struct message *msg);

#endif /* MESSAGE_SENDER_H */
