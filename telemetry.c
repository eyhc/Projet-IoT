#include "telemetry.h"

#include "lpsxxx.h"
#include "lpsxxx_params.h"

#include "lis2dh12.h"
#include "lis2dh12_params.h"
#include "lis2dh12_registers.h"

#include "cayenne_lpp.h"

#include "chat.h"
#include "fmt.h"
#include "message.h"
#include "mutex.h"
#include "xtimer.h"
#include "ztimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TELEMETRY_THREAD_STACK_SIZE (512U)

static lpsxxx_t lps_dev;
static lis2dh12_t lis_dev;

static uint16_t publish_preiod_s = 60U;
static bool publish_broadcast = false;
static char publish_group[4] = {'\0'};

void telemetry_set_period(uint16_t period_s) { publish_preiod_s = period_s; }
void telemetry_set_dest_group(char name[4]) { name_cpy(publish_group, name); }
void telemetry_set_broadcast(int broadcast) { publish_broadcast = broadcast; }

static void sprint_lpp_cayenne(struct telemetry *t, size_t size,
                               char out[size]) {
  cayenne_lpp_t lpp;
  cayenne_lpp_add_barometric_pressure(&lpp, 1, (float)t->pressure);
  cayenne_lpp_add_temperature(&lpp, 2, (float)t->temperature / 100.0f);

  char str_out[3][8];

  size_t len = fmt_s16_dfp(str_out[0], t->acc_x, -3);
  str_out[0][len] = '\0';
  len = fmt_s16_dfp(str_out[1], t->acc_y, -3);
  str_out[1][len] = '\0';
  len = fmt_s16_dfp(str_out[2], t->acc_z, -3);
  str_out[2][len] = '\0';

  cayenne_lpp_add_accelerometer(&lpp, 3, atof(str_out[0]), atof(str_out[1]),
                                atof(str_out[2]));

  for (uint8_t i = 0; i < lpp.cursor && i < size / 2; i++) {
    sprintf(out + (i * 2), "%02X", lpp.buffer[i]);
  }

  out[lpp.cursor * 2] = '\0';
}

static int telemetry_thread_signal = 0;
static kernel_pid_t _telemetry_pid;
static char stack[TELEMETRY_THREAD_STACK_SIZE];
static struct message msg;
void *_telemetry_thread(void *arg) {
  struct sync_chat_data *shared_data = (struct sync_chat_data *)arg;

  puts("Telemetry thread started");

  while (1) {
    if (telemetry_thread_signal == 1) {
      puts("Telemetry thread stopping...");
      return NULL;
    }

    mutex_lock(shared_data->mutex);
    if (shared_data->chat_data->local_user.name[0] == '\0') {
      mutex_unlock(shared_data->mutex);
      ztimer_sleep(ZTIMER_SEC, publish_preiod_s);
      continue;
    }

    msg.counter = ++shared_data->chat_data->local_user.last_seen_counter;
    name_cpy(msg.sender, shared_data->chat_data->local_user.name);
    mutex_unlock(shared_data->mutex);

    struct telemetry t;
    telemetry_read(&t);
    sprint_lpp_cayenne(&t, sizeof(msg.content) - 4, msg.content + 3);
    msg.content[0] = 'l';
    msg.content[1] = 'p';
    msg.content[2] = 'p';
    msg.content[MAX_MESSAGE_SIZE] = '\0';

    if (publish_broadcast) {
      msg.dest_type = DEST_BROADCAST;
      msg.dest[0] = '\0';
      chat_send_message(&msg);
    } else if (publish_group[0] != '\0') {
      msg.dest_type = DEST_GROUP;
      name_cpy(msg.dest, publish_group);
      chat_send_message(&msg);
    }

    ztimer_sleep(ZTIMER_SEC, publish_preiod_s);
  }
}

void telemetry_init(void) {
  printf("Initializing %s sensor\n", LPSXXX_SAUL_NAME);
  if (lpsxxx_init(&lps_dev, &lpsxxx_params[0]) != LPSXXX_OK) {
    puts("LPSXXX Initialization failed");
  }

  lis2dh12_init(&lis_dev, &lis2dh12_params[0]);
  lis2dh12_poweron(&lis_dev);
}

void telemetry_start(void) {
  telemetry_thread_signal = 0;

  // Start telemetry thread
  _telemetry_pid = thread_create(
      stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
      _telemetry_thread, (void *)get_shared_chat_data(), "telemetry_thread");

  if (_telemetry_pid <= KERNEL_PID_UNDEF) {
    puts("Creation of telemetry thread failed");
  }
}

void telemetry_stop(void) { telemetry_thread_signal = 1; }

void telemetry_read(struct telemetry *t) {
  lpsxxx_enable(&lps_dev);
  xtimer_sleep(1);

  lpsxxx_read_pres(&lps_dev, &t->pressure);
  lpsxxx_read_temp(&lps_dev, &t->temperature);
  lpsxxx_disable(&lps_dev);

  lis2dh12_fifo_data_t data;
  lis2dh12_read(&lis_dev, &data);

  t->acc_x = data.axis.x;
  t->acc_y = data.axis.y;
  t->acc_z = data.axis.z;
}

void telemetry_print(void) {
  struct telemetry t;
  telemetry_read(&t);

  int temp_int = t.temperature / 100;
  int temp_dec = t.temperature % 100;
  printf("Pressure value: %ihPa - Temperature: %2i.%02i°C\n", t.pressure,
         temp_int, temp_dec);

  char str_out[3][8];

  size_t len = fmt_s16_dfp(str_out[0], t.acc_x, -3);
  str_out[0][len] = '\0';
  len = fmt_s16_dfp(str_out[1], t.acc_y, -3);
  str_out[1][len] = '\0';
  len = fmt_s16_dfp(str_out[2], t.acc_z, -3);
  str_out[2][len] = '\0';

  printf("X: %6s  Y: %6s  Z: %6s\n", str_out[0], str_out[1], str_out[2]);
}

void telemetry_print_lpp_cayenne(void) {
  static char out[2 * CAYENNE_LPP_MAX_BUFFER_SIZE + 1];
  struct telemetry t;
  telemetry_read(&t);

  sprint_lpp_cayenne(&t, sizeof(out), out);
  printf("LPP CAYENNE: %s\n", out);
}

int telemetry_cmd(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s [start|stop|print|print_lpp_cayenne|set]\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "start") == 0) {
    telemetry_start();
  } else if (strcmp(argv[1], "stop") == 0) {
    telemetry_stop();
  } else if (strcmp(argv[1], "print") == 0) {
    telemetry_print();
  } else if (strcmp(argv[1], "print_lpp_cayenne") == 0) {
    telemetry_print_lpp_cayenne();
  } else if (strcmp(argv[1], "set") == 0) {
    if (argc != 4) {
      printf("Usage: %s set [period|broadcast|group] <value>\n", argv[0]);
      return 2;
    }

    if (strcmp(argv[2], "broadcast") == 0) {
      telemetry_set_broadcast(atoi(argv[3]) != 0);
      telemetry_set_dest_group("\0\0\0\0");
      printf("Telemetry broadcast set to %s\n",
             atoi(argv[3]) != 0 ? "ON" : "OFF");
      printf("Group destination cleared\n");
    } else if (strcmp(argv[2], "group") == 0) {
      if (strlen(argv[3]) >= 4) {
        telemetry_set_broadcast(0);
        telemetry_set_dest_group(argv[3]);
        printf("Telemetry destination group set to '%s'\n", argv[3]);
        printf("Broadcast disabled\n");
      } else {
        printf("Invalid group name. Must be at least 4 characters.\n");
        return 3;
      }
    } else if (strcmp(argv[2], "period") == 0) {
      telemetry_set_period(atoi(argv[3]));
      printf("Telemetry publish period set to %s seconds\n", argv[3]);
    } else {
      printf("Invalid setting. Use 'period', 'broadcast', or 'group'.\n");
      return 4;
    }
  } else {
    printf("Usage: %s [start|stop|print|print_lpp_cayenne|set_period]\n",
           argv[0]);
    return 1;
  }

  return 0;
}
