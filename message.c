#include "message.h"

#include "lora.h"
#include <string.h>

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

static char
    buffer[MAX_MESSAGE_SIZE + 20]; // buffer pour stocker la chaîne formatée
void print_message(const struct message *msg) {
  sprint_message(sizeof(buffer), buffer, msg);
  printf("%s\n", buffer);
}

int send_message(struct message *msg) {
  sprint_message(sizeof(buffer), buffer, msg);
  return lora_send(2, (char *[]){"lora_send", buffer});
}
