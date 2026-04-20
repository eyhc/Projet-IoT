#include "contact.h"
#include "chat_data.h"

// La specification des fonctions suivantes sont définies dans contact.h

void list_contacts(struct sync_chat_data *data) {
  puts("List of view contacts:");

  mutex_lock(data->mutex);
  print_contact_table(MAX_CONTACTS, data->chat_data->chat_contacts);
  mutex_unlock(data->mutex);
}

/* ========================================================================== */

void list_favorite_contacts(struct sync_chat_data *data) {
  printf("Favorite contacts:\n");
  mutex_lock(data->mutex);

  for (size_t i = 0; i < MAX_CONTACTS; i++) {
    if (data->chat_data->chat_contacts[i].name[0] != '\0' &&
        data->chat_data->chat_contacts[i].is_favorite) {
      printf("  %u: Name:%.4s, last_seen_counter:%lu\n", i,
             data->chat_data->chat_contacts[i].name,
             data->chat_data->chat_contacts[i].last_seen_counter);
    }
  }

  mutex_unlock(data->mutex);
}

int add_favorite_contact(struct sync_chat_data *data, const char *name) {
  mutex_lock(data->mutex);

  int idx = get_contact_index(data->chat_data->chat_contacts, name);
  if (idx == -1) { // ajout du contact si le contact n'existe pas encore
    idx = get_contact_empty_index(data->chat_data->chat_contacts);

    if (idx == -1)
      return 1; // no empty slot

    name_cpy(data->chat_data->chat_contacts[idx].name, name);
    data->chat_data->chat_contacts[idx].last_seen_counter = 0;
  }

  data->chat_data->chat_contacts[idx].is_favorite = 1;
  mutex_unlock(data->mutex);
  return 0;
}

int remove_favorite_contact(struct sync_chat_data *data, const char *name) {
  mutex_lock(data->mutex);
  int idx = get_contact_index(data->chat_data->chat_contacts, name);
  if (idx == -1) { // contact not found
    mutex_unlock(data->mutex);
    return 3;
  }

  if (!data->chat_data->chat_contacts[idx]
           .is_favorite) { // contact found but not favorite
    mutex_unlock(data->mutex);
    return 2;
  }

  data->chat_data->chat_contacts[idx].is_favorite = 0;
  mutex_unlock(data->mutex);
  return 0;
}
