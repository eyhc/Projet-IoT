#include "chat.h"

#include "contact.h"
#include "eeprom.h"
#include "group.h"
#include "lora.h"
#include "message.h"
#include "thread.h"
#include "ztimer.h"
#include <stdio.h>
#include <string.h>

// NB. La specification des fonctions suivantes sont définies dans chat.h

static mutex_t chat_mutex;
static struct chat_data chat_data;
static struct sync_chat_data shared_data = {.chat_data = &chat_data,
                                            .mutex = &chat_mutex};

struct sync_chat_data *get_shared_chat_data(void) { return &shared_data; }

#define SAVE_THREAD_STACK_SIZE (256U)

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

  init_sx1272(0, NULL);
  lora_channel(3, (char *[]){"channel", "set", channel_str});
  lora_setup(4, (char *[]){"lora_setup", bandwidth_str, spreading_factor_str,
                           code_rate_str});
  lora_crc(3, (char *[]){"lora_crc", "set", crc_str});
  lora_implicit(3, (char *[]){"lora_implicit", "set", implicit_header_str});
  lora_syncword(3, (char *[]){"lora_syncword", "set", syncword_str});

  lora_set_message_callback((lora_message_callback_t)main_listen_message_entry);
  lora_listen(0, NULL);
}

/* ====================== INFO / CONTACTS / GROUPES ====================== */

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
    list_favorite_contacts(&shared_data);
  }

  /* ----------- ADD FAVORITE ----------- */
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

    int res = add_favorite_contact(&shared_data, contact_name);
    if (res == 1) {
      printf("Error: contact list is full\n");
      return 3;
    }

    printf("Contact '%s' added to favorites successfully\n", contact_name);
  }

  /* ----------- RM FAVORITE ----------- */
  else {
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

    int res = remove_favorite_contact(&shared_data, contact_name);
    if (res == 1) {
      printf("Error: contact not found\n");
      return 3;
    } else if (res == 2) {
      printf("Error: contact is not a favorite\n");
      return 4;
    }

    printf("Contact '%s' removed from favorites successfully\n", contact_name);
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
    list_groups(&shared_data);
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

    int res = join_group(&shared_data, group_name);
    if (res == 1) {
      printf("Error: already joined to this group\n");
      return 4;
    } else if (res == 2) {
      printf("Error: group list is full\n");
      return 5;
    }

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

    int res = leave_group(&shared_data, group_name);
    if (res == 1) {
      printf("Error: not joined to this group\n");
      return 4;
    }

    printf("Group '%s' removed successfully\n", group_name);
  }

  return 0;
}

int chat_contact(int argc, char *argv[argc]) {
  if (argc != 2 || strcmp(argv[1], "ls") != 0) {
    printf("Usage: %s ls\n", argv[0]);
    return 1;
  }

  list_contacts(&shared_data);
  return 0;
}

/* ====================== ENVOIE DE MESSAGES ====================== */

int chat_send_message(struct message *msg) {
  static char buffer[MAX_MESSAGE_SIZE + 20];
  sprint_message(sizeof(buffer), buffer, msg);
  return lora_send(2, (char *[]){"lora_send", buffer});
}

int chat_send_broadcast_cmd(int argc, char *argv[argc]) {
  if (argc != 2) {
    printf("Usage: %s <message>\n", argv[0]);
    return 1;
  }

  mutex_lock(shared_data.mutex);
  if (shared_data.chat_data->local_user.name[0] == '\0') {
    printf("Error: username not set yet\n");
    mutex_unlock(shared_data.mutex);
    return 2;
  }

  struct message msg;
  name_cpy(msg.sender, shared_data.chat_data->local_user.name);
  msg.counter = ++shared_data.chat_data->local_user.last_seen_counter;
  mutex_unlock(shared_data.mutex);

  msg.dest_type = DEST_BROADCAST;
  msg.dest[0] = '\0';
  strncpy(msg.content, argv[1], MAX_MESSAGE_SIZE);
  msg.content[MAX_MESSAGE_SIZE] = '\0';

  return chat_send_message(&msg);
}

int chat_send_to_contact_cmd(int argc, char *argv[argc]) {
  if (argc != 4 ||
      (strcmp(argv[1], "alias") != 0 && strcmp(argv[1], "id") != 0)) {
    printf("Usage: %s [alias|id] <contact_name|contact_id> <message>\n",
           argv[0]);
    return 1;
  }

  mutex_lock(shared_data.mutex);
  if (shared_data.chat_data->local_user.name[0] == '\0') {
    printf("Error: username not set yet\n");
    mutex_unlock(shared_data.mutex);
    return 2;
  }
  mutex_unlock(shared_data.mutex);

  struct message msg;
  mutex_lock(shared_data.mutex);
  name_cpy(msg.sender, shared_data.chat_data->local_user.name);
  msg.counter = ++shared_data.chat_data->local_user.last_seen_counter;
  mutex_unlock(shared_data.mutex);

  if (strcmp(argv[1], "alias") == 0) {
    msg.dest_type = DEST_CONTACT;
    name_cpy(msg.dest, argv[2]);
  } else {
    mutex_lock(shared_data.mutex);
    if (atoi(argv[2]) < 0 || atoi(argv[2]) >= MAX_CONTACTS ||
        shared_data.chat_data->chat_contacts[atoi(argv[2])].name[0] == '\0') {
      printf("Error: invalid contact id\n");
      mutex_unlock(shared_data.mutex);
      return 3;
    }
    name_cpy(msg.dest,
             shared_data.chat_data->chat_contacts[atoi(argv[2])].name);
    mutex_unlock(shared_data.mutex);

    msg.dest_type = DEST_CONTACT;
  }

  strncpy(msg.content, argv[3], MAX_MESSAGE_SIZE);
  msg.content[MAX_MESSAGE_SIZE] = '\0';
  return chat_send_message(&msg);
}

int chat_send_to_group_cmd(int argc, char *argv[argc]) {
  if (argc != 4 ||
      (strcmp(argv[1], "alias") != 0 && strcmp(argv[1], "id") != 0)) {
    printf("Usage: %s [alias|id] <group_name|group_id> <message>\n", argv[0]);
    return 1;
  }

  mutex_lock(shared_data.mutex);
  if (shared_data.chat_data->local_user.name[0] == '\0') {
    printf("Error: username not set yet\n");
    mutex_unlock(shared_data.mutex);
    return 2;
  }
  mutex_unlock(shared_data.mutex);

  struct message msg;
  mutex_lock(shared_data.mutex);
  name_cpy(msg.sender, shared_data.chat_data->local_user.name);
  msg.counter = ++shared_data.chat_data->local_user.last_seen_counter;
  mutex_unlock(shared_data.mutex);

  if (strcmp(argv[1], "alias") == 0) {
    name_cpy(msg.dest, argv[2]);
    msg.dest_type = DEST_GROUP;

    mutex_lock(shared_data.mutex);
    int idx = get_group_index(shared_data.chat_data->chat_groups, msg.dest);
    mutex_unlock(shared_data.mutex);

    if (idx == -1 && join_group(&shared_data, msg.dest) == 2) {
      printf("Error: failed to join group\n");
      return 3;
    }
  } else {
    mutex_lock(shared_data.mutex);
    if (atoi(argv[2]) < 0 || atoi(argv[2]) >= MAX_GROUPS ||
        shared_data.chat_data->chat_groups[atoi(argv[2])][0] == '\0') {
      printf("Error: invalid group id\n");
      mutex_unlock(shared_data.mutex);
      return 3;
    }

    name_cpy(msg.dest, shared_data.chat_data->chat_groups[atoi(argv[2])]);
    mutex_unlock(shared_data.mutex);

    msg.dest_type = DEST_GROUP;
  }

  strncpy(msg.content, argv[3], MAX_MESSAGE_SIZE);
  msg.content[MAX_MESSAGE_SIZE] = '\0';

  return chat_send_message(&msg);
}
