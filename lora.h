#ifndef LORA_H
#define LORA_H

#include <stdint.h>

int init_sx1272(int argc, char **argv);
int lora_setup(int argc, char **argv);
int lora_implicit(int argc, char **argv);
int lora_crc(int argc, char **argv);
int lora_payload(int argc, char **argv);
int lora_syncword(int argc, char **argv);
int lora_channel(int argc, char **argv);
int lora_send(int argc, char **argv);
int lora_listen(int argc, char **argv);
int lora_reset(int argc, char **argv);
uint32_t lora_random(void);

#endif /* LORA_H */
