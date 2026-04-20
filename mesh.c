#include "mesh.h"

#include <stdio.h>
#include <string.h>

static int mesh_enabled = 0;

void mesh_enable(int enable) { mesh_enabled = enable; }
int mesh_is_enabled(void) { return mesh_enabled; }

/* -------------------------------------------------------------- */

static int8_t snr_threshold;
static uint16_t max_ttl;

void mesh_init(void) {
  snr_threshold = DEFAULT_SNR_THRESHOLD;
  max_ttl = DEFAULT_TTL;
}

void mesh_set_snr_threshold(int8_t threshold) { snr_threshold = threshold; }
void mesh_set_ttl(uint16_t ttl) { max_ttl = ttl; }
uint16_t mesh_get_ttl(void) { return max_ttl; }

int mesh_set_cmd(int argc, char **argv) {
  if (argc != 4 || strcmp(argv[1], "set") != 0) {
    printf("Usage: %s set [snr_threshold|ttl] <value>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[2], "snr_threshold") == 0) {
    mesh_set_snr_threshold((int8_t)atoi(argv[3]));
    printf("Mesh SNR threshold set to %d dB\n", snr_threshold);
  } else if (strcmp(argv[2], "ttl") == 0) {
    mesh_set_ttl((uint16_t)atoi(argv[3]));
    printf("Mesh TTL set to %d\n", max_ttl);
  } else {
    printf("Invalid setting. Use 'snr_threshold' or 'ttl'.\n");
    return 2;
  }

  return 0;
}

int mesh_print_queue(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("Mesh message queue display not implemented yet.\n");
  return 0;
}
