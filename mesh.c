#include "mesh.h"

#include "architecture.h"
#include "lora.h"
#include "mutex.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

#define MESH_QUEUE_SIZE 10

enum status { RELAYED, NOT_RELAYED };

struct queue_entry {
  struct received_message msg;
  uint32_t relay_time;
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

#define MIN_RELAY_DELAY 1000  // ms
#define MAX_RELAY_DELAY 10000 // ms
#define RANDOM_JITTER 1000    // ms

static uint32_t relayDelay(uint32_t ToA, int16_t snr) {

  // delay = ToA + (snr_threshold - snr) * K + random_jitter

  float snr_factor = 0.0f;
  if (snr < snr_threshold)
    snr_factor = (snr_threshold - snr) * 200.0f;

  uint32_t jitter = lora_random() % RANDOM_JITTER;
  uint32_t delay = (uint32_t)(snr_factor) + ToA + jitter;

  delay = (delay < MIN_RELAY_DELAY) ? MIN_RELAY_DELAY : delay;
  delay = (delay > MAX_RELAY_DELAY) ? MAX_RELAY_DELAY : delay;
  return delay;
}

/* -------------------------------------------------------------------------- */

mutex_t mesh_queue_mutex = MUTEX_INIT;

void mesh_handle_message(struct received_message *msg) {
  if (!mesh_is_enabled())
    return;

  if (msg->snr > snr_threshold) {
    printf("Message SNR above threshold (%d dB), not relaying.\n",
           snr_threshold);
    return;
  }

  if (msg->msg.ttl <= 0) {
    printf("Message TTL expired, not relaying.\n");
    return;
  }

  uint32_t delay = relayDelay(msg->toa, msg->snr);
  struct queue_entry entry = {
      .msg = *msg, .relay_time = delay, .relay_status = NOT_RELAYED};

  // check if message is already in the queue (to avoid duplicates)
  mutex_lock(&mesh_queue_mutex);
  size_t i;
  for (i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
    struct queue_entry *e = &mesh_queue[i];
    if (e->msg.msg.counter == msg->msg.counter)
      break;
  }
  mutex_unlock(&mesh_queue_mutex);

  if (i == head) {
    if (is_queue_full()) {
      printf("Mesh relay queue full, cannot dequeue first entry.\n");
      queue_dequeue(NULL);
      return;
    }
    queue_enqueue(&entry);
  }
}

int mesh_print_queue(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("Mesh Relay Queue:\n");
  for (size_t i = tail; i != head; i = (i + 1) % MESH_QUEUE_SIZE) {
    struct queue_entry *entry = &mesh_queue[i];
    printf("  [%u] Sender: %.4s, Dest: %.4s, Content: %s, TTL: %li, Relay "
           "Delay: %lu ms, status: %s\n",
           i, entry->msg.msg.sender, entry->msg.msg.dest,
           entry->msg.msg.content, entry->msg.msg.ttl, entry->relay_time,
           entry->relay_status == RELAYED ? "RELAYED" : "NOT_RELAYED");
  }
  return 0;
}

/* -------------------------------------------------------------------------- */

static int mesh_enabled = 0;

void mesh_enable(int enable) { mesh_enabled = enable; }
int mesh_is_enabled(void) { return mesh_enabled; }

/*
#define MESH_THREAD_STACK_SIZE 1024
static kernel_pid_t _mesh_pid;
static char stack[MESH_THREAD_STACK_SIZE];
void _mesh_task(void *arg) {
  (void)arg;

  while (1) {
  }
}
*/
