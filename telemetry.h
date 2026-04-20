#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "architecture.h"

struct telemetry {
  uint16_t pressure;
  int16_t temperature;
  int16_t acc_x;
  int16_t acc_y;
  int16_t acc_z;
};

// Initialise les modules de télémétrie (capteurs LPSXXX et LIS2DH12)
void telemetry_init(void);

// Lit les données de télémétrie à partir des capteurs et les stocke dans la
// structure fournie
void telemetry_read(struct telemetry *t);

// Affiche les données de télémétrie actuelles (pression, température,
// accéléromètre)
void telemetry_print(void);

// Affiche les données de télémétrie actuelles au format Cayenne LPP
void telemetry_print_lpp_cayenne(void);

// Modifie la période d'émission télémétrique à la valeur spécifiée en secondes
void telemetry_set_period(uint16_t period_s);

// Définit le nom du groupe dans lequel les données de télémétrie seront
// publiées.
// Pour ne pas publier dans un groupe, utiliser name[0] = '\0'
void telemetry_set_dest_group(char name[4]);

// Définit si les données de télémétrie seront publiées en broadcast.
// 0 pour désactiver le broadcast, toute autre valeur pour l'activer
void telemetry_set_broadcast(int broadcast);

// Démarre l'émission télémétrique périodique
void telemetry_start(void);

// Arrête l'émission télémétrique périodique
void telemetry_stop(void);

// Commande shell pour gérer la télémétrie (affichage, configuration, etc.)
int telemetry_cmd(int argc, char **argv);

#endif /* TELEMETRY_H */
