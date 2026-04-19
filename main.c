#include "architecture.h"
#include "chat.h"
#include "eeprom.h"
#include "lora.h"
#include "shell.h"

#define SAVE_PERIOD_SEC (600U)

static const shell_command_t shell_commands[] = {
    {"lora_setup", "Initialize LoRa modulation settings", lora_setup_cmd},
    {"lora_implicit", "Enable implicit header", lora_implicit_cmd},
    {"lora_crc", "Enable CRC", lora_crc_cmd},
    {"lora_payload", "Set payload length (implicit header)", lora_payload_cmd},
    {"lora_syncword", "Get/Set the syncword", lora_syncword_cmd},
    {"lora_channel", "Get/Set channel frequency (in Hz)", lora_channel_cmd},
    {"lora_send", "Send raw payload string", lora_send_cmd},
    {"lora_listen", "Start raw payload listener", lora_listen_cmd},
    {"lora_reset", "Reset the sx127x device", lora_reset_cmd},
    {"eeprom_print", "Print the content of the EEPROM", eeprom_print_data_cmd},
    {"username", "Get/Set the chat username", chat_username},
    {NULL, NULL, NULL}};

int main(void) {
  /* initialisation du chat (+ lora + eeprom) */
  chat_init(SAVE_PERIOD_SEC);

  /* start the shell */
  puts("Initialization successful - starting the shell now");
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
