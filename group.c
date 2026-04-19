#include "group.h"

// La specification des fonctions suivantes sont définies dans group.h

void list_groups(struct sync_chat_data *data) {
  mutex_lock(data->mutex);
  puts("List of joined groups:");
  print_group_table(MAX_GROUPS, data->chat_data->chat_groups);
  mutex_unlock(data->mutex);
}

int join_group(struct sync_chat_data *data, const char *group_name) {
  mutex_lock(data->mutex);

  int idx = get_group_index(data->chat_data, group_name);
  if (idx != -1) { // group already exists
    mutex_unlock(data->mutex);
    return 1;
  }

  idx = get_group_empty_index(data->chat_data);
  if (idx == -1) { // no empty slot
    mutex_unlock(data->mutex);
    return 2;
  }

  name_cpy(data->chat_data->chat_groups[idx], group_name);

  mutex_unlock(data->mutex);
  return 0;
}

int leave_group(struct sync_chat_data *data, const char *group_name) {
  mutex_lock(data->mutex);
  int idx = get_group_index(data->chat_data, group_name);
  if (idx == -1) { // group not found
    mutex_unlock(data->mutex);
    return 1;
  }

  data->chat_data->chat_groups[idx][0] = '\0';
  mutex_unlock(data->mutex);
  return 0;
}
