#include "chat.h"

#include "eeprom.h"
#include "lora.h"
#include "thread.h"
#include "ztimer.h"
#include <stdio.h>
#include <string.h>

// NB. La specification des fonctions suivantes sont définies dans chat.h

static mutex_t chat_mutex;
static struct chat_data chat_data;
static struct sync_chat_data shared_data = {.chat_data = &chat_data,
                                            .mutex = &chat_mutex};

#define SAVE_THREAD_STACK_SIZE (512)

// Thread de sauvegarde périodique des données dans l'EEPROM
static uint16_t period_sec = 10;
static kernel_pid_t _save_pid;
static char stack[SAVE_THREAD_STACK_SIZE];
void *_save_thread(void *arg) {
  (void)arg;

  while (1) {
    ztimer_sleep(ZTIMER_SEC, period_sec);
    mutex_lock(shared_data.mutex);
    eeprom_write_data(shared_data.chat_data);
    mutex_unlock(shared_data.mutex);
    puts("Chat data saved to EEPROM");
  }
}

void chat_init(uint32_t period_s) {
  /* -- Initialisation EEPROM -- */
  assert_eeprom_enough_space();

  mutex_lock(shared_data.mutex);
  eeprom_read_data(shared_data.chat_data);

  if (shared_data.chat_data->magic_word != MAGIC_WORD) {
    printf("EEPROM not initialized, writing default values...\n");
    shared_data.chat_data->magic_word = MAGIC_WORD;
    shared_data.chat_data->lora_channel = 868299987;
    shared_data.chat_data->lora_bw = 125;
    shared_data.chat_data->lora_sf = 7;
    shared_data.chat_data->lora_cr = 5;
    shared_data.chat_data->lora_crc = 1;
    shared_data.chat_data->lora_implicit = 0;
    shared_data.chat_data->lora_syncword = 0x22;
    eeprom_write_data(shared_data.chat_data);
  }
  mutex_unlock(shared_data.mutex);

  /* -- Thread de sauvegarde des données -- */
  period_sec = period_s;
  _save_pid =
      thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                    THREAD_CREATE_STACKTEST, _save_thread, NULL, "save_thread");

  if (_save_pid <= KERNEL_PID_UNDEF) {
    puts("Creation of save thread failed");
  }

  /* -- Initialisation du module LORA -- */
  char bandwidth_str[16];
  char code_rate_str[16];
  char spreading_factor_str[16];
  char implicit_header_str[16];
  char crc_str[16];
  char syncword_str[16];
  char channel_str[16];

  mutex_lock(shared_data.mutex);
  snprintf(bandwidth_str, sizeof(bandwidth_str), "%u",
           shared_data.chat_data->lora_bw);
  snprintf(code_rate_str, sizeof(code_rate_str), "%u",
           shared_data.chat_data->lora_cr);
  snprintf(spreading_factor_str, sizeof(spreading_factor_str), "%u",
           shared_data.chat_data->lora_sf);
  snprintf(implicit_header_str, sizeof(implicit_header_str), "%u",
           shared_data.chat_data->lora_implicit);
  snprintf(crc_str, sizeof(crc_str), "%u", shared_data.chat_data->lora_crc);
  snprintf(syncword_str, sizeof(syncword_str), "%u",
           shared_data.chat_data->lora_syncword);
  snprintf(channel_str, sizeof(channel_str), "%lu",
           shared_data.chat_data->lora_channel);
  mutex_unlock(shared_data.mutex);

  init_sx1272_cmd(0, NULL);
  lora_channel_cmd(3, (char *[]){"channel", "set", channel_str});
  lora_setup_cmd(4, (char *[]){"lora_setup", bandwidth_str,
                               spreading_factor_str, code_rate_str});
  lora_crc_cmd(3, (char *[]){"lora_crc", "set", crc_str});
  lora_implicit_cmd(3, (char *[]){"lora_implicit", "set", implicit_header_str});
  lora_syncword_cmd(3, (char *[]){"lora_syncword", "set", syncword_str});
}

int chat_info(int argc, char *argv[argc]) {
  (void)argc;
  (void)argv;

  puts("Chat information:");
  mutex_lock(shared_data.mutex);
  print_chat_data(shared_data.chat_data);
  mutex_unlock(shared_data.mutex);

  return 0;
}

int chat_username(int argc, char *argv[argc]) {
  if (argc < 2 ||
      (strcmp(argv[1], "get") != 0 && strcmp(argv[1], "set") != 0)) {
    printf("Usage: %s <get|set>\n", argv[0]);
    return -1;
  }

  if (strcmp(argv[1], "get") == 0) { // GET HANDLER
    mutex_lock(shared_data.mutex);
    if (shared_data.chat_data->local_user.name[0] == '\0') {
      printf("Username not set yet\n");
    } else {
      printf("Current username: %.4s\n",
             shared_data.chat_data->local_user.name);
    }
    mutex_unlock(shared_data.mutex);
  } else { // SET HANDLER
    if (argc != 3) {
      printf("Usage: %s set <username>\n", argv[0]);
      return 1;
    }

    const char *new_username = argv[2];
    if (strlen(new_username) != NAME_SIZE) {
      printf("Error: username must be exactly %d characters long\n", NAME_SIZE);
      return 2;
    }

    mutex_lock(shared_data.mutex);
    name_cpy(shared_data.chat_data->local_user.name, new_username);
    mutex_unlock(shared_data.mutex);

    printf("Username set to '%s'\n", new_username);
  }

  return 0;
}

int chat_favorite(int argc, char *argv[argc]) {
  if (argc < 2 || (strcmp(argv[1], "add") != 0 && strcmp(argv[1], "rm") != 0 &&
                   strcmp(argv[1], "ls") != 0)) {
    printf("Usage: %s <add|rm|ls>\n", argv[0]);
    return 1;
  }

  /* ----------- LS FAVORITES ----------- */
  if (strcmp(argv[1], "ls") == 0) {
    printf("Favorite contacts:\n");
    mutex_lock(shared_data.mutex);

    for (size_t i = 0; i < MAX_CONTACTS; i++)
      if (shared_data.chat_data->chat_contacts[i].is_favorite)
        printf("  Name=%.4s, last_seen_counter=%lu\n",
               shared_data.chat_data->chat_contacts[i].name,
               shared_data.chat_data->chat_contacts[i].last_seen_counter);

    mutex_unlock(shared_data.mutex);
  }

  /* ----------- ADD GROUP ----------- */
  else if (strcmp(argv[1], "add") == 0) {
    if (argc != 3) {
      printf("Usage: %s add <contact_name>\n", argv[0]);
      return 2;
    }

    const char *contact_name = argv[2];
    if (strlen(contact_name) != NAME_SIZE) {
      printf("Error: contact name must be exactly %d characters long\n",
             NAME_SIZE);
      return 2;
    }

    mutex_lock(shared_data.mutex);
    int idx = get_contact_index(shared_data.chat_data, contact_name);
    if (idx == -1) { // ajout du contact si le contact n'existe pas encore
      idx = 0;
      while (idx < MAX_CONTACTS &&
             shared_data.chat_data->chat_contacts[idx].name[0] != '\0')
        idx++;

      if (idx == MAX_CONTACTS) {
        mutex_unlock(shared_data.mutex);
        printf("Error: contact list is full\n");
        return 3;
      }

      name_cpy(shared_data.chat_data->chat_contacts[idx].name, contact_name);
      shared_data.chat_data->chat_contacts[idx].last_seen_counter = 0;
    }

    shared_data.chat_data->chat_contacts[idx].is_favorite = 1;
    mutex_unlock(shared_data.mutex);

    printf("Contact '%s' added to favorites successfully\n", contact_name);
  } else {
    if (argc != 3) {
      printf("Usage: %s rm <contact_name>\n", argv[0]);
      return 2;
    }

    const char *contact_name = argv[2];
    if (strlen(contact_name) != NAME_SIZE) {
      printf("Error: contact name must be exactly %d characters long\n",
             NAME_SIZE);
      return 2;
    }

    mutex_lock(shared_data.mutex);
    int idx = get_contact_index(shared_data.chat_data, contact_name);
    if (idx == -1) {
      mutex_unlock(shared_data.mutex);
      printf("Error: contact not found\n");
      return 3;
    } else {
      shared_data.chat_data->chat_contacts[idx].is_favorite = 0;
      mutex_unlock(shared_data.mutex);

      printf("Contact '%s' removed from favorites successfully\n",
             contact_name);
    }
  }

  return 0;
}

int chat_group(int argc, char *argv[argc]) {
  if (argc < 2 || (strcmp(argv[1], "add") != 0 && strcmp(argv[1], "rm") != 0 &&
                   strcmp(argv[1], "ls") != 0)) {
    printf("Usage: %s <add|rm|ls>\n", argv[0]);
    return 1;
  }

  /* ----------- LS GROUPS ----------- */
  if (strcmp(argv[1], "ls") == 0) {
    mutex_lock(shared_data.mutex);
    print_group_table(MAX_GROUPS, shared_data.chat_data->chat_groups);
    mutex_unlock(shared_data.mutex);
  }

  /* ----------- ADD GROUP ----------- */
  else if (strcmp(argv[1], "add") == 0) {
    if (argc != 3) {
      printf("Usage: %s add <group_name>\n", argv[0]);
      return 2;
    }

    const char *group_name = argv[2];
    if (strlen(group_name) != NAME_SIZE) {
      printf("Error: group name must be exactly %d characters long\n",
             NAME_SIZE);
      return 3;
    }

    mutex_lock(shared_data.mutex);
    int idx = get_group_index(shared_data.chat_data, group_name);
    if (idx != -1) {
      mutex_unlock(shared_data.mutex);
      printf("Error: group already exists\n");
      return 4;
    }

    idx = 0;
    while (idx < MAX_GROUPS &&
           shared_data.chat_data->chat_groups[idx][0] != '\0') {
      idx++;
    }

    if (idx == MAX_GROUPS) {
      mutex_unlock(shared_data.mutex);
      printf("Error: group list is full\n");
      return 5;
    }

    name_cpy(shared_data.chat_data->chat_groups[idx], group_name);
    mutex_unlock(shared_data.mutex);
    printf("Group '%s' added successfully\n", group_name);
  }

  /* ----------- RM GROUP ----------- */
  else {
    if (argc != 3) {
      printf("Usage: %s rm <group_name>\n", argv[0]);
      return 2;
    }

    const char *group_name = argv[2];
    if (strlen(group_name) != NAME_SIZE) {
      printf("Error: group name must be exactly %d characters long\n",
             NAME_SIZE);
      return 3;
    }

    mutex_lock(shared_data.mutex);
    int idx = get_group_index(shared_data.chat_data, group_name);
    if (idx == -1) {
      mutex_unlock(shared_data.mutex);
      printf("Error: group not found\n");
      return 4;
    } else {
      shared_data.chat_data->chat_groups[idx][0] = '\0';
      mutex_unlock(shared_data.mutex);
      printf("Group '%s' removed successfully\n", group_name);
    }
  }

  return 0;
}

int chat_contact(int argc, char *argv[argc]) {
  if (argc != 2 || strcmp(argv[1], "ls") != 0) {
    printf("Usage: %s ls\n", argv[0]);
    return 1;
  }

  puts("List of view contacts:");

  mutex_lock(shared_data.mutex);
  print_contact_table(MAX_CONTACTS, shared_data.chat_data->chat_contacts);
  mutex_unlock(shared_data.mutex);

  return 0;
}
