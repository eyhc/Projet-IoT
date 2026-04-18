#include "eeprom.h"

#include "periph/eeprom.h"
#include "test_utils/expect.h"
#include "thread.h"
#include "ztimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// La spec des fonctions suivantes sont définies dans eeprom.h.
void assert_eeprom_enough_space(void) {
  if (sizeof(struct eeprom_data) > EEPROM_SIZE) {
    puts("Error: struct eeprom_data size exceeds EEPROM size.");
  }
}

// ici on définit les données communes à tout le système et sauvegardées dans
// l'EEPROM.
static struct eeprom_data data;
static mutex_t eeprom_mutex = MUTEX_INIT;
struct shared_eeprom_data shared_data = {.data = &data, .mutex = &eeprom_mutex};

// Thread de sauvegarde périodique des données dans l'EEPROM
static uint16_t period_sec = 10;
static kernel_pid_t _save_pid;

static char stack[512];
void *_save_thread(void *arg) {
  (void)arg;

  while (1) {
    mutex_lock(shared_data.mutex);
    eeprom_write_data();
    mutex_unlock(shared_data.mutex);
    ztimer_sleep(ZTIMER_SEC, period_sec);
  }
}

void init_eeprom(uint32_t period_s) {
  assert_eeprom_enough_space();

  period_sec = period_s;

  eeprom_read_data();

  mutex_lock(shared_data.mutex);
  if (data.magic_word != EEPROM_MAGIC_WORD) {
    printf("EEPROM not initialized, writing default values...\n");
    data.magic_word = EEPROM_MAGIC_WORD;
    data.lora_channel = 868299987;
    data.lora_bw = 125;
    data.lora_sf = 7;
    data.lora_cr = 5;
    data.lora_crc = 1;
    data.lora_implicit = 0;
    data.lora_syncword = 0x22;
    mutex_unlock(shared_data.mutex);
    eeprom_write_data();
  } else {
    mutex_unlock(shared_data.mutex);
  }

  _save_pid =
      thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                    THREAD_CREATE_STACKTEST, _save_thread, NULL, "save_thread");

  if (_save_pid <= KERNEL_PID_UNDEF) {
    puts("Creation of save thread failed");
    return 1;
  }
}

void eeprom_write_data(void) {
  mutex_lock(shared_data.mutex);
  eeprom_write(0, (uint8_t *)&data, sizeof(struct eeprom_data));
  mutex_unlock(shared_data.mutex);
}

void eeprom_read_data(void) {
  mutex_lock(shared_data.mutex);
  eeprom_read(0, (uint8_t *)&data, sizeof(struct eeprom_data));
  mutex_unlock(shared_data.mutex);
}

void print_contact_table(size_t size, struct chat_contact c[size]) {
  for (size_t i = 0; i < size; i++) {
    if (c[i].name[0] != '\0') {
      printf("  Name=%.4s, last_seen_counter=%lu\n", c[i].name,
             (unsigned long)c[i].last_seen_counter);
    }
  }
}

// Lit les données de l'EEPROM au format struct eeprom_data,
// et affiche les données lues à l'écran de manière lisible (pour le debug)
int eeprom_print_data_cmd(int argc, char **argv) {
  (void)argc;
  (void)argv;

  eeprom_read_data();

  mutex_lock(shared_data.mutex);

  puts("EEPROM data:");
  printf("Magic word: %u\n", data.magic_word);

  printf("LoRa channel: %u\n", data.lora_channel);
  printf("LoRa bandwidth: %u\n", data.lora_bw);
  printf("LoRa spreading factor: %u\n", data.lora_sf);
  printf("LoRa coding rate: %u\n", data.lora_cr);
  printf("LoRa CRC enabled: %u\n", data.lora_crc);
  printf("LoRa implicit header: %u\n", data.lora_implicit);
  printf("LoRa syncword: %u\n", data.lora_syncword);

  printf("Local user name: %.4s\n", data.local_user.name);
  printf("Local user last seen counter: %lu\n",
         (unsigned long)data.local_user.last_seen_counter);

  printf("Chat favorites:\n");
  print_contact_table(MAX_FAVORITES, data.chat_favorites);

  printf("Chat contacts:\n");
  print_contact_table(MAX_CONTACTS, data.chat_contacts);

  printf("Chat groups:\n");
  print_contact_table(MAX_GROUPS, data.chat_groups);

  mutex_unlock(shared_data.mutex);

  return 0;
}
