#ifndef MESH_H
#define MESH_H

#include "message.h"

#define DEFAULT_SNR_THRESHOLD (10) // en dB
#define DEFAULT_TTL (5)            // nombre de sauts maximum

// Initialise le module de communication en maillage (mesh)
void mesh_init(void);

// Traite un message reçu : décide s'il doit être réémis ou non en fonction du
// seuil de SNR et du TTL, et le place en queue de réémission si nécessaire avec
// le bon délai d'attente.
void mesh_handle_message(struct received_message *msg);

// Configuration du seuil du SNR.
// Aucun message au dessus de ce seuil ne sera réémis.
void mesh_set_snr_threshold(int8_t threshold);

// Configuration du TTL maximum des messages émis ou
// réémis.
void mesh_set_ttl(uint16_t max_ttl);

// Récupère le TTL maximum configuré pour les messages émis ou réémis.
uint16_t mesh_get_ttl(void);

// Commandes shell pour la configuration
int mesh_set_cmd(int argc, char **argv);

// Commandes shell pour l'affichage de la queue de messages en attente de
// réémission.
int mesh_print_queue(int argc, char **argv);

// Active ou désactive le module de communication en maillage (mesh).
void mesh_enable(int enable);

// Retourne 1 si le module de communication en maillage (mesh) est activé, 0
// sinon.
int mesh_is_enabled(void);

#endif /* MESH_H */
