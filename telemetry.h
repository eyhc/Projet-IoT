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
void telemetry_change_publish_period(uint16_t period_s);

// Démarre l'émission télémétrique périodique
void telemetry_start(void);

// Arrête l'émission télémétrique périodique
void telemetry_stop(void);

#endif /* TELEMETRY_H */
