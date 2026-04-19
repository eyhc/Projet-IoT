/*
 * Ce fichier définit les données du système qui seront stockées dans l'EEPROM,
 * ainsi que les fonctions pour lire et écrire ces données.
 *
 * Auteur : apalma
 */
#ifndef EEPROM_H
#define EEPROM_H

#include "architecture.h"
#include "chat_data.h"

// Fonctions pour lire et écrire les données de l'EEPROM
void eeprom_write_data(struct chat_data *data);
void eeprom_read_data(struct chat_data *data);

// Fonction à utiliser pour s'assurer que la structure de données tient bien
// dans l'EEPROM. Si ce n'est pas le cas, il faut réduire la taille des tableaux
// de contacts ou des groupes
void assert_eeprom_enough_space(void);

// Lit les données de l'EEPROM au format struct eeprom_data,
// et affiche les données lues à l'écran de manière lisible (pour le debug)
int eeprom_print_data_cmd(int argc, char **argv);

#endif /* EEPROM_H */
