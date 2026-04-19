#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include <stdlib.h>

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

typedef void (*lora_message_callback_t)(size_t len, char *message, int16_t rssi,
                                        int8_t snr, uint32_t toa);
void lora_set_message_callback(lora_message_callback_t callback);

#endif /* LORA_H */
