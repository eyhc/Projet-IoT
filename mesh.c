#include "mesh.h"

#include "architecture.h"
#include "lora.h"
#include "mutex.h"
#include "ztimer.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
// FIFO DES MESSAGES (queue circulaire)

#define MESH_QUEUE_SIZE 10

enum status { RELAYED, NOT_RELAYED };

struct queue_entry {
  struct received_message msg;
  uint32_t relay_time;
  uint32_t relay_remaining_time;
  enum status relay_status;
};

uint32_t tail = 0;
uint32_t head = 0;

struct queue_entry mesh_queue[MESH_QUEUE_SIZE];

static int is_queue_full(void) {
  size_t next = (head + 1) % MESH_QUEUE_SIZE;
  return next == tail;
}

static int is_queue_empty(void) { return tail == head; }

int queue_enqueue(struct queue_entry *entry) {
  // if full, cannot enqueue
  if (is_queue_full())
    return 0;

  // otherwise, enqueue the byte
  mesh_queue[head] = *entry;
  head = (head + 1) % MESH_QUEUE_SIZE;
  return 1;
}

int queue_dequeue(struct queue_entry *entry) {
  // if empty, cannot dequeue
  if (is_queue_empty())
    return 0;

  // otherwise, dequeue the byte
  if (entry)
    *entry = mesh_queue[tail];
  tail = (tail + 1) % MESH_QUEUE_SIZE;
  return 1;
}

/* -------------------------------------------------------------- */
// MODIFICATION DES PARAMETRES DU RESEAU MESH

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

/* ------------------------------------------------------------------------- */
// Calcul du délai de relais d'un message en fonction de son ToA et de son SNR

#define MIN_RELAY_DELAY 1000  // ms
#define MAX_RELAY_DELAY 10000 // ms
#define RANDOM_JITTER 2000    // ms

static uint32_t relayDelay(uint32_t ToA, int16_t snr) {

  // delay = ToA + (snr_threshold - snr) * K + random_jitter

  float snr_factor = 0.0f;
  if (snr < snr_threshold)
    snr_factor = (snr_threshold - snr) * 500.0f;

  uint32_t jitter = lora_random() % RANDOM_JITTER;
  uint32_t delay = (uint32_t)(snr_factor) + ToA + jitter;

  delay = (delay < MIN_RELAY_DELAY) ? MIN_RELAY_DELAY : delay;
  delay = (delay > MAX_RELAY_DELAY) ? MAX_RELAY_DELAY : delay;
  return delay;
}

/* -------------------------------------------------------------------------- */
// Ajoute les message reçus / émis à la queue de relais s'ils passent les
// conditions

void mesh_handle_message(struct received_message *msg) {
  if (!mesh_is_enabled())
    return;

  if (msg->snr > snr_threshold) {
    printf("Message SNR above threshold (%d dB), not relaying.\n",
           snr_threshold);
    return;
  }

  if (msg->msg.ttl <= 0) {
    return;
  }

  uint32_t delay = relayDelay(msg->toa, msg->snr);
  msg->msg.ttl--;
  struct queue_entry entry = {.msg = *msg,
                              .relay_time = delay,
                              .relay_remaining_time = delay,
                              .relay_status = NOT_RELAYED};

  // check if message is already in the queue (to avoid duplicates)
  size_t i;
  for (i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
    struct queue_entry *e = &mesh_queue[i];
    if (e->msg.msg.counter == msg->msg.counter)
      break;
  }

  if (i == head) {
    if (is_queue_full()) {
      if (mesh_queue[head].relay_status == NOT_RELAYED)
        printf("Mesh relay queue full, cannot dequeue first entry.\n");
      queue_dequeue(NULL);
      return;
    }
    queue_enqueue(&entry);
  }
}

// Commande shell pour afficher la queue de relais (pour debug)
int mesh_print_queue(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("Mesh Relay Queue:\n");
  for (size_t i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
    struct queue_entry *entry = &mesh_queue[i];
    printf("  [%u] Sender: %.4s, Dest: %.4s, Content: %s, TTL: %li, Relay "
           "Delay: %lu ms, remaining delay: %lu ms, status: %s\n",
           i, entry->msg.msg.sender, entry->msg.msg.dest,
           entry->msg.msg.content, entry->msg.msg.ttl, entry->relay_time,
           entry->relay_remaining_time,
           entry->relay_status == RELAYED ? "RELAYED" : "NOT_RELAYED");
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
// THREAD DE RELAIS QUI RELAYE LES MESSAGES DE LA QUEUE QUAND LEUR DELAI EST
// ECHECUE

static int mesh_enabled = 0;

#define MESH_THREAD_STACK_SIZE 1024
static kernel_pid_t _mesh_pid;
static char stack[MESH_THREAD_STACK_SIZE];
void *_mesh_task(void *arg) {
  (void)arg;

  puts("Mesh thread started.\n");

  static uint32_t wait_time = 0;
  while (1) {
    if (!mesh_enabled) {
      puts("Mesh thread stopped.\n");
      return NULL;
    }

    // Relayer tous les messages qui ont atteint leur délai de relais
    size_t t = 0;
    for (size_t i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
      struct queue_entry *entry = &mesh_queue[i];
      entry->relay_remaining_time =
          (entry->relay_remaining_time > wait_time)
              ? entry->relay_remaining_time - wait_time
              : 0;
      if (entry->relay_status == NOT_RELAYED) {
        if (entry->relay_remaining_time <= 0) {
          printf("relay message: ");
          static char buffer[MAX_MESSAGE_SIZE + 20];
          sprint_message(sizeof(buffer), buffer, &entry->msg.msg);
          lora_send(2, (char *[]){"lora_send", buffer});
          entry->relay_status = RELAYED;
          t++;
          ztimer_sleep(ZTIMER_MSEC, 500); // small delay between relays
        }
      }
    }

    // chercher le delay_min pour le prochain message à relayer
    wait_time = 1000;
    for (size_t i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
      struct queue_entry *entry = &mesh_queue[i];
      entry->relay_remaining_time = (entry->relay_remaining_time > t * 500)
                                        ? entry->relay_remaining_time - t * 500
                                        : 0;
      if (entry->relay_status == NOT_RELAYED) {
        if (wait_time == 0 || entry->relay_remaining_time < wait_time)
          wait_time = entry->relay_remaining_time;
      }
    }

    ztimer_sleep(ZTIMER_MSEC, wait_time);
  }
}

void mesh_enable(int enable) {
  if (enable && !mesh_enabled) {
    mesh_enabled = 1;
    _mesh_pid =
        thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                      THREAD_CREATE_STACKTEST, _mesh_task, NULL, "mesh_thread");
    if (_mesh_pid <= KERNEL_PID_UNDEF) {
      puts("Creation of mesh thread failed");
    }
  }
  mesh_enabled = enable;
}

int mesh_is_enabled(void) { return mesh_enabled; }
