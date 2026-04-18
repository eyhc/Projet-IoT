#include "architecture.h"
#include "eeprom.h"
#include "lora.h"
#include "shell.h"

#define SAVE_PERIOD_SEC (600U)

static void init(void) {
  // Setup du module LoRa avec les paramètres stockés dans l'EEPROM
  char bandwidth_str[16];
  char code_rate_str[16];
  char spreading_factor_str[16];
  char implicit_header_str[16];
  char crc_str[16];
  char syncword_str[16];
  char channel_str[16];

  snprintf(bandwidth_str, sizeof(bandwidth_str), "%u",
           shared_data.data->lora_bw);
  snprintf(code_rate_str, sizeof(code_rate_str), "%u",
           shared_data.data->lora_cr);
  snprintf(spreading_factor_str, sizeof(spreading_factor_str), "%u",
           shared_data.data->lora_sf);
  snprintf(implicit_header_str, sizeof(implicit_header_str), "%u",
           shared_data.data->lora_implicit);
  snprintf(crc_str, sizeof(crc_str), "%u", shared_data.data->lora_crc);
  snprintf(syncword_str, sizeof(syncword_str), "%u",
           shared_data.data->lora_syncword);
  snprintf(channel_str, sizeof(channel_str), "%u",
           shared_data.data->lora_channel);
  lora_channel_cmd(2, (char *[]){"channel", "set", channel_str});
  lora_setup_cmd(4, (char *[]){"lora_setup", bandwidth_str,
                               spreading_factor_str, code_rate_str});
  lora_crc_cmd(3, (char *[]){"lora_crc", "set", crc_str});
  lora_implicit_cmd(3, (char *[]){"lora_implicit", "set", implicit_header_str});
  lora_syncword_cmd(3, (char *[]){"lora_syncword", "set", syncword_str});
}

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
    {NULL, NULL, NULL}};

int main(void) {
  init_sx1272_cmd(0, NULL);
  init_eeprom(SAVE_PERIOD_SEC);
  init();

  /* start the shell */
  puts("Initialization successful - starting the shell now");
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
