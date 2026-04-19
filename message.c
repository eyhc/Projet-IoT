#include "message.h"

#include "chat.h"
#include "lora.h"
#include <string.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))

static char temp_buffer[MAX_MESSAGE_SIZE + 1];
void sprint_message(size_t buffer_size, char buffer[buffer_size],
                    const struct message *msg) {

  const char *content = msg->content;
  if (msg->content[MAX_MESSAGE_SIZE] != '\0') {
    content = temp_buffer;
    memcpy(temp_buffer, msg->content, MAX_MESSAGE_SIZE);
    temp_buffer[MAX_MESSAGE_SIZE] = '\0';
  }

  uint32_t ctr = msg->counter;
  switch (msg->dest_type) {
  case DEST_BROADCAST:
    snprintf(buffer, buffer_size, "%.4s@*:%lu:%s", msg->sender, ctr, content);
    break;
  case DEST_CONTACT:
    snprintf(buffer, buffer_size, "%.4s@%.4s:%lu:%s", msg->sender, msg->dest,
             ctr, content);
    break;
  case DEST_GROUP:
    snprintf(buffer, buffer_size, "%.4s#%.4s:%lu:%s", msg->sender, msg->dest,
             ctr, content);
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
  for (; i < size && buffer[i] != ':'; j++, i++) {
    temp[j] = buffer[i];
  }
  temp[j] = '\0';
  if (i >= size)
    return 5;
  msg->counter = atoll(temp);
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

void main_listen_message_entry(size_t len, char *message, int16_t rssi,
                               int8_t snr, uint32_t toa) {

  struct received_message rcv_msg;

  if (parse_message(len, message, &rcv_msg.msg) != 0) {
    return;
  }

  rcv_msg.rssi = rssi;
  rcv_msg.snr = snr;
  rcv_msg.toa = toa;

  struct sync_chat_data *shared_data = get_shared_chat_data();
  if (filter_message(&rcv_msg.msg, shared_data)) {
    // add_message_to_my_history(&rcv_msg.msg, shared_data);

    char to = (rcv_msg.msg.dest_type != DEST_GROUP) ? '@' : '#';
    printf("[%.4s -> %c%.4s]: %s, RSSI=%i, SNR=%i, ToA=%lu\n",
           rcv_msg.msg.sender, to, rcv_msg.msg.dest, rcv_msg.msg.content,
           rcv_msg.rssi, rcv_msg.snr, rcv_msg.toa);
  } else {
    // add_message_to_other_dest_history(&rcv_msg.msg, shared_data);
  }

  // update last seen counter for this contact
  mutex_lock(shared_data->mutex);
  int idx = get_contact_index(shared_data->chat_data->chat_contacts,
                              rcv_msg.msg.sender);
  if (idx != -1) {
    shared_data->chat_data->chat_contacts[idx].last_seen_counter =
        max(shared_data->chat_data->chat_contacts[idx].last_seen_counter,
            rcv_msg.msg.counter);
  } else {
    // contact inconnu, on l'ajoute à la liste des contacts
    int empty_idx =
        get_contact_empty_index(shared_data->chat_data->chat_contacts);
    if (empty_idx != -1) {
      name_cpy(shared_data->chat_data->chat_contacts[empty_idx].name,
               rcv_msg.msg.sender);
      shared_data->chat_data->chat_contacts[empty_idx].last_seen_counter =
          rcv_msg.msg.counter;
      shared_data->chat_data->chat_contacts[empty_idx].is_favorite = 0;
    } else {
      printf("Warning: no space to add new contact %.4s\n", rcv_msg.msg.sender);
    }
  }
  mutex_unlock(shared_data->mutex);
}
