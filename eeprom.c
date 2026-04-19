#include "eeprom.h"

#include "periph/eeprom.h"
#include <stdio.h>

// La spec des fonctions suivantes sont définies dans eeprom.h.
void assert_eeprom_enough_space(void) {
  if (sizeof(struct chat_data) > EEPROM_SIZE) {
    puts("Error: struct chat_data size exceeds EEPROM size.");
  }
}

void eeprom_write_data(struct chat_data *data) {
  eeprom_write(0, (uint8_t *)data, sizeof(struct chat_data));
}

void eeprom_read_data(struct chat_data *data) {
  eeprom_read(0, (uint8_t *)data, sizeof(struct chat_data));
}

// Lit les données de l'EEPROM au format struct eeprom_data,
// et affiche les données lues à l'écran de manière lisible (pour le debug)
static struct chat_data data;
int eeprom_print_data_cmd(int argc, char **argv) {
  (void)argc;
  (void)argv;
  eeprom_read_data(&data);

  puts("EEPROM data:");
  printf("Magic word: %u\n", data.magic_word);

  print_chat_data(&data);

  return 0;
}
