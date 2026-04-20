#include "message.h"

#include "chat.h"
#include "lora.h"
#include <string.h>

static char temp_buffer[MAX_MESSAGE_SIZE + 1];
static char ttl_buffer[10];
void sprint_message(size_t buffer_size, char buffer[buffer_size],
                    const struct message *msg) {

  const char *content = msg->content;
  if (msg->content[MAX_MESSAGE_SIZE] != '\0') {
    content = temp_buffer;
    memcpy(temp_buffer, msg->content, MAX_MESSAGE_SIZE);
    temp_buffer[MAX_MESSAGE_SIZE] = '\0';
  }

  if (msg->ttl == -1) {
    ttl_buffer[0] = '\0';
  } else {
    snprintf(ttl_buffer, sizeof(ttl_buffer), ",%ld", msg->ttl);
  }

  uint32_t ctr = msg->counter;
  switch (msg->dest_type) {
  case DEST_BROADCAST:
    snprintf(buffer, buffer_size, "%.4s@*:%lu%s:%s", msg->sender, ctr,
             ttl_buffer, content);
    break;
  case DEST_CONTACT:
    snprintf(buffer, buffer_size, "%.4s@%.4s:%lu%s:%s", msg->sender, msg->dest,
             ctr, ttl_buffer, content);
    break;
  case DEST_GROUP:
    snprintf(buffer, buffer_size, "%.4s#%.4s:%lu%s:%s", msg->sender, msg->dest,
             ctr, ttl_buffer, content);
    break;
  }
}

static char buffer[MAX_MESSAGE_SIZE + 20];
void print_message(const struct message *msg) {
  sprint_message(sizeof(buffer), buffer, msg);
  printf("%s\n", buffer);
}

// ============================================================================

int parse_message(size_t size, char buffer[size], struct message *msg) {
  static char temp[32];

  // SENDER
  size_t i = 0;
  for (; i < NAME_SIZE && i < size && buffer[i] != '@' && buffer[i] != '#';
       i++) {
    temp[i] = buffer[i];
  }
  temp[NAME_SIZE] = '\0';

  if (strlen(temp) != NAME_SIZE)
    return 1;
  name_cpy(msg->sender, temp);

  for (; i < size && buffer[i] != '@' && buffer[i] != '#';
       i++) // cherche prochain marqueur
    ;

  if (buffer[i] == '@') {
    msg->dest_type = DEST_CONTACT;
  } else if (buffer[i] == '#') {
    msg->dest_type = DEST_GROUP;
  } else {
    return 2;
  }
  i++;

  // DEST
  if (buffer[i] == '*') {
    if (msg->dest_type == DEST_GROUP) // broadcast ne peut pas être un groupe
      return 3;

    msg->dest_type = DEST_BROADCAST;
    msg->dest[0] = '*';
    msg->dest[1] = '\0';
  } else {
    for (size_t j = 0; j < NAME_SIZE && i < size && buffer[i] != ':';
         j++, i++) {
      temp[j] = buffer[i];
    }
    temp[NAME_SIZE] = '\0';
    if (strlen(temp) != NAME_SIZE)
      return 4;
    name_cpy(msg->dest, temp);
  }

  for (; i < size && buffer[i] != ':'; i++) // va jusqu'au prochain ':'
    ;
  i++;

  // COUNTER
  size_t j = 0;
  for (;
       j < sizeof(temp) - 1 && i < size && buffer[i] != ':' && buffer[i] != ',';
       j++, i++) {
    temp[j] = buffer[i];
  }
  temp[j] = '\0';
  if (i >= size)
    return 5;
  msg->counter = atoll(temp);

  // TTL (optional)
  if (buffer[i] == ',') {
    i++;
    for (j = 0; j < sizeof(temp) - 1 && i < size && buffer[i] != ':';
         j++, i++) {
      temp[j] = buffer[i];
    }
    temp[j] = '\0';
    int32_t ttl = atoi(temp);
    if (ttl < 0 || ttl > 65535)
      return 6;
    msg->ttl = ttl;
  } else {
    msg->ttl = -1;
  }
  i++;

  // CONTENT
  size_t content_size = size - i;
  if (content_size > MAX_MESSAGE_SIZE)
    return 6;
  memcpy(msg->content, buffer + i, content_size);
  msg->content[content_size] = '\0';

  return 0;
}

int filter_message(const struct message *msg,
                   const struct sync_chat_data *data) {

  int res = 0;
  mutex_lock(data->mutex);
  int idx = get_contact_index(data->chat_data->chat_contacts, msg->sender);
  if (idx != -1 &&
      msg->counter <= data->chat_data->chat_contacts[idx].last_seen_counter) {
    // message déjà vu, on ignore
    res = 0;
  } else {
    if (msg->dest_type == DEST_BROADCAST) {
      res = 1;
    } else if (msg->dest_type == DEST_CONTACT) {
      res = name_cmp(msg->dest, data->chat_data->local_user.name);
    } else if (msg->dest_type == DEST_GROUP) {
      for (size_t i = 0; i < MAX_GROUPS && res == 0; i++)
        if (name_cmp(msg->dest, data->chat_data->chat_groups[i]))
          res = 1;
    }
  }
  mutex_unlock(data->mutex);

  return res;
}
