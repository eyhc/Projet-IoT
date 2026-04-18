#ifndef LORA_H
#define LORA_H

#include <stdint.h>

int init_sx1272_cmd(int argc, char **argv);
int lora_setup_cmd(int argc, char **argv);
int lora_implicit_cmd(int argc, char **argv);
int lora_crc_cmd(int argc, char **argv);
int lora_payload_cmd(int argc, char **argv);
int lora_syncword_cmd(int argc, char **argv);
int lora_channel_cmd(int argc, char **argv);
int lora_send_cmd(int argc, char **argv);
int lora_listen_cmd(int argc, char **argv);
int lora_reset_cmd(int argc, char **argv);
uint32_t lora_random(void);

#endif /* LORA_H */
