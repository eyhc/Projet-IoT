#include "architecture.h"
#include "chat.h"
#include "eeprom.h"
#include "lora.h"
#include "mesh.h"
#include "shell.h"
#include "telemetry.h"
#include <stdio.h>
#include <string.h>

#define SAVE_PERIOD_SEC (600U)

int lora_setup_cmd(int argc, char **argv) {
  int res = lora_setup(argc, argv);

  if (res == 0) {
    struct sync_chat_data *shared_data = get_shared_chat_data();
    mutex_lock(shared_data->mutex);
    shared_data->chat_data->lora_bw = atoi(argv[1]);
    shared_data->chat_data->lora_sf = atoi(argv[2]);
    shared_data->chat_data->lora_cr = atoi(argv[3]);
    mutex_unlock(shared_data->mutex);
  }

  return res;
}

int lora_implicit_cmd(int argc, char **argv) {
  int res = lora_implicit(argc, argv);

  if (res == 0) {
    struct sync_chat_data *shared_data = get_shared_chat_data();
    mutex_lock(shared_data->mutex);
    shared_data->chat_data->lora_implicit = atoi(argv[2]);
    mutex_unlock(shared_data->mutex);
  }

  return res;
}

int lora_crc_cmd(int argc, char **argv) {
  int res = lora_crc(argc, argv);

  if (res == 0) {
    struct sync_chat_data *shared_data = get_shared_chat_data();
    mutex_lock(shared_data->mutex);
    shared_data->chat_data->lora_crc = atoi(argv[2]);
    mutex_unlock(shared_data->mutex);
  }

  return res;
}

int lora_syncword_cmd(int argc, char **argv) {
  int res = lora_syncword(argc, argv);

  if (res == 0) {
    struct sync_chat_data *shared_data = get_shared_chat_data();
    mutex_lock(shared_data->mutex);
    shared_data->chat_data->lora_syncword = atoi(argv[2]);
    mutex_unlock(shared_data->mutex);
  }

  return res;
}

int lora_channel_cmd(int argc, char **argv) {
  int res = lora_channel(argc, argv);

  if (res == 0) {
    struct sync_chat_data *shared_data = get_shared_chat_data();
    mutex_lock(shared_data->mutex);
    shared_data->chat_data->lora_channel = atol(argv[2]);
    mutex_unlock(shared_data->mutex);
  }

  return res;
}

int rx_msg_cmd(int argc, char **argv) {
  if (argc != 2 && argc != 5) {
    printf("Usage: %s <message> <rssi> <snr> <toa>\n", argv[0]);
    printf("   or: %s <message>\n", argv[0]);
    return -1;
  }

  size_t len = strlen(argv[1]);
  if (len > MAX_MESSAGE_SIZE) {
    printf("Message too long (max %d characters)\n", MAX_MESSAGE_SIZE);
    return -1;
  }

  uint8_t rssi = (argc == 5) ? (uint8_t)atoi(argv[2]) : 0;
  int8_t snr = (argc == 5) ? (int8_t)atoi(argv[3]) : 0;
  uint32_t toa = (argc == 5) ? (uint32_t)atoi(argv[4]) : 0;

  printf("Simulating reception of message '%s' with RSSI=%d, SNR=%d, ToA=%lu\n",
         argv[1], rssi, snr, toa);

  chat_listen_message(len, argv[1], rssi, snr, toa);
  return 0;
}

int mesh_cmd(int argc, char **argv) {
  if (argc < 2 ||
      (strcmp(argv[1], "set") != 0 && strcmp(argv[1], "print") != 0 &&
       strcmp(argv[1], "enable") != 0 && strcmp(argv[1], "disable") != 0)) {
    printf("Usage: %s [set|print|enable|disable]\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "set") == 0) {
    return mesh_set_cmd(argc, argv);
  } else if (strcmp(argv[1], "enable") == 0) {
    mesh_enable(1);
    return 0;
  } else if (strcmp(argv[1], "disable") == 0) {
    mesh_enable(0);
    return 0;
  } else {
    return mesh_print_queue(argc, argv);
  }
}

static const shell_command_t shell_commands[] = {
    {"lora_listen", "Start listening for LoRa messages", lora_listen},
    {"lora_setup", "Initialize LoRa modulation settings", lora_setup_cmd},
    {"lora_implicit", "Enable implicit header", lora_implicit_cmd},
    {"lora_crc", "Enable CRC", lora_crc_cmd},
    {"lora_syncword", "Get/Set the syncword", lora_syncword_cmd},
    {"lora_channel", "Get/Set channel frequency (in Hz)", lora_channel_cmd},
    {"eeprom_print", "Print the content of the EEPROM", eeprom_print_data_cmd},
    {"info", "Print system information", chat_info},
    {"username", "Get/Set the chat username", chat_username},
    {"favorite", "Manage favorite contacts", chat_favorite},
    {"group", "Manage discussion groups", chat_group},
    {"contact", "Manage contacts", chat_contact},
    {"send_broadcast", "Send a broadcast message", chat_send_broadcast_cmd},
    {"send_contact", "Send a message to a contact", chat_send_to_contact_cmd},
    {"send_group", "Send a message to a group", chat_send_to_group_cmd},
    {"force_rcv", "Simulate the reception of a msg (for testing)", rx_msg_cmd},
    {"telemetry", "Manage telemetry", telemetry_cmd},
    {"mesh", "Manage mesh settings", mesh_cmd},
    {NULL, NULL, NULL}};

int main(void) {
  /* initialisation du chat (+ lora + eeprom) */
  chat_init(SAVE_PERIOD_SEC);
  mesh_init();
  telemetry_init();

  /* start the shell */
  puts("Initialization successful - starting the shell now");
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
