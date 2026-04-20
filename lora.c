/*
 * Copyright (C) 2016 Unwired Devices <info@unwds.com>
 *               2017 Inria Chile
 *               2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 * @file
 * @brief       Test application for SX127X modem driver
 *
 * @author      Eugene P. <ep@unwds.com>
 * @author      José Ignacio Alamos <jose.alamos@inria.cl>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */
#include "lora.h"

#include "message.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"
// #include "shell_commands.h"

#include "net/lora.h"
#include "net/netdev.h"
#include "net/netdev/lora.h"

#include "board.h"

#include "sx127x_internal.h"
#include "sx127x_netdev.h"
#include "sx127x_params.h"

#include "fmt.h"

#define SX127X_LORA_MSG_QUEUE (16U)
#ifndef SX127X_STACKSIZE
#define SX127X_STACKSIZE (THREAD_STACKSIZE_DEFAULT)
#endif

#define MSG_TYPE_ISR (0x3456)

static char stack[SX127X_STACKSIZE];
static kernel_pid_t _recv_pid;

static char message[MAX_MESSAGE_SIZE + 30];
static sx127x_t sx127x;

int lora_setup(int argc, char **argv) {
  if (argc < 4) {
    puts("usage: setup "
         "<bandwidth (125, 250, 500)> "
         "<spreading factor (7..12)> "
         "<code rate (5..8)>");
    return -1;
  }

  /* Check bandwidth value */
  int bw = atoi(argv[1]);
  uint8_t lora_bw;

  switch (bw) {
  case 125:
    puts("setup: setting 125KHz bandwidth");
    lora_bw = LORA_BW_125_KHZ;
    break;

  case 250:
    puts("setup: setting 250KHz bandwidth");
    lora_bw = LORA_BW_250_KHZ;
    break;

  case 500:
    puts("setup: setting 500KHz bandwidth");
    lora_bw = LORA_BW_500_KHZ;
    break;

  default:
    puts("[Error] setup: invalid bandwidth value given, "
         "only 125, 250 or 500 allowed.");
    return -1;
  }

  /* Check spreading factor value */
  uint8_t lora_sf = atoi(argv[2]);

  if (lora_sf < 7 || lora_sf > 12) {
    puts("[Error] setup: invalid spreading factor value given");
    return -1;
  }

  /* Check coding rate value */
  int cr = atoi(argv[3]);

  if (cr < 5 || cr > 8) {
    puts("[Error ]setup: invalid coding rate value given");
    return -1;
  }
  uint8_t lora_cr = (uint8_t)(cr - 4);

  /* Configure radio device */
  netdev_t *netdev = &sx127x.netdev;

  netdev->driver->set(netdev, NETOPT_BANDWIDTH, &lora_bw, sizeof(lora_bw));
  netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &lora_sf,
                      sizeof(lora_sf));
  netdev->driver->set(netdev, NETOPT_CODING_RATE, &lora_cr, sizeof(lora_cr));

  puts("[Info] setup: configuration set with success");

  return 0;
}

uint32_t lora_random(void) {
  netdev_t *netdev = &sx127x.netdev;
  uint32_t rand;

  netdev->driver->get(netdev, NETOPT_RANDOM, &rand, sizeof(rand));
  // printf("random: number from sx127x: %u\n", (unsigned int)rand);

  /* reinit the transceiver to default values */
  sx127x_init_radio_settings(&sx127x);

  return rand;
}

int lora_send(int argc, char **argv) {
  if (argc <= 1) {
    puts("usage: send <payload>");
    return -1;
  }

  printf("sending \"%s\" payload (%u bytes)\n", argv[1],
         (unsigned)strlen(argv[1]) + 1);

  iolist_t iolist = {.iol_base = argv[1], .iol_len = (strlen(argv[1]) + 1)};

  netdev_t *netdev = &sx127x.netdev;

  if (netdev->driver->send(netdev, &iolist) == -ENOTSUP) {
    puts("Cannot send: radio is still transmitting");
  }

  return 0;
}

int lora_listen(int argc, char **argv) {
  (void)argc;
  (void)argv;

  netdev_t *netdev = &sx127x.netdev;
  /* Switch to continuous listen mode */
  const netopt_enable_t single = false;

  netdev->driver->set(netdev, NETOPT_SINGLE_RECEIVE, &single, sizeof(single));
  const uint32_t timeout = 0;

  netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(timeout));

  /* Switch to RX state */
  netopt_state_t state = NETOPT_STATE_RX;

  netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));

  printf("Listen mode set\n");

  return 0;
}

int lora_syncword(int argc, char **argv) {
  if (argc < 2) {
    puts("usage: syncword <get|set>");
    return -1;
  }

  netdev_t *netdev = &sx127x.netdev;
  uint8_t syncword;

  if (strstr(argv[1], "get") != NULL) {
    netdev->driver->get(netdev, NETOPT_SYNCWORD, &syncword, sizeof(syncword));
    printf("Syncword: 0x%02x\n", syncword);
    return 0;
  }

  if (strstr(argv[1], "set") != NULL) {
    if (argc < 3) {
      puts("usage: syncword set <syncword>");
      return -1;
    }
    syncword = fmt_hex_byte(argv[2]);
    netdev->driver->set(netdev, NETOPT_SYNCWORD, &syncword, sizeof(syncword));
    printf("Syncword set to %02x\n", syncword);
  } else {
    puts("usage: syncword <get|set>");
    return -1;
  }

  return 0;
}

int lora_channel(int argc, char **argv) {
  if (argc < 2) {
    puts("usage: channel <get|set>");
    return -1;
  }

  netdev_t *netdev = &sx127x.netdev;
  uint32_t chan;

  if (strstr(argv[1], "get") != NULL) {
    netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
    printf("Channel: %i\n", (int)chan);
    return 0;
  }

  if (strstr(argv[1], "set") != NULL) {
    if (argc < 3) {
      puts("usage: channel set <channel>");
      return -1;
    }
    chan = atoi(argv[2]);
    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
    printf("New channel set to %lu\n", chan);
  } else {
    puts("usage: channel <get|set>");
    return -1;
  }

  return 0;
}

int lora_reset(int argc, char **argv) {
  (void)argc;
  (void)argv;
  netdev_t *netdev = &sx127x.netdev;

  puts("resetting sx127x...");
  netopt_state_t state = NETOPT_STATE_RESET;

  netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(netopt_state_t));
  return 0;
}

static void _set_opt(netdev_t *netdev, netopt_t opt, bool val, char *str_help) {
  netopt_enable_t en = val ? NETOPT_ENABLE : NETOPT_DISABLE;

  netdev->driver->set(netdev, opt, &en, sizeof(en));
  printf("Successfully ");
  if (val) {
    printf("enabled ");
  } else {
    printf("disabled ");
  }
  printf("%s\n", str_help);
}

int lora_crc(int argc, char **argv) {
  netdev_t *netdev = &sx127x.netdev;

  if (argc < 3 || strcmp(argv[1], "set") != 0) {
    printf("usage: %s set <1|0>\n", argv[0]);
    return 1;
  }

  int tmp = atoi(argv[2]);

  _set_opt(netdev, NETOPT_INTEGRITY_CHECK, tmp, "CRC check");
  return 0;
}

int lora_implicit(int argc, char **argv) {
  netdev_t *netdev = &sx127x.netdev;

  if (argc < 3 || strcmp(argv[1], "set") != 0) {
    printf("usage: %s set <1|0>\n", argv[0]);
    return 1;
  }

  int tmp = atoi(argv[2]);

  _set_opt(netdev, NETOPT_FIXED_HEADER, tmp, "implicit header");
  return 0;
}

int lora_payload(int argc, char **argv) {
  netdev_t *netdev = &sx127x.netdev;

  if (argc < 3 || strcmp(argv[1], "set") != 0) {
    printf("usage: %s set <payload length>\n", argv[0]);
    return 1;
  }

  uint16_t tmp = atoi(argv[2]);

  netdev->driver->set(netdev, NETOPT_PDU_SIZE, &tmp, sizeof(tmp));
  printf("Successfully set payload to %i\n", tmp);
  return 0;
}

// =======================================================================

static lora_message_callback_t _message_callback = NULL;

void lora_set_message_callback(lora_message_callback_t callback) {
  _message_callback = callback;
}

static void _event_cb(netdev_t *dev, netdev_event_t event) {
  if (event == NETDEV_EVENT_ISR) {
    msg_t msg;

    msg.type = MSG_TYPE_ISR;
    msg.content.ptr = dev;

    if (msg_send(&msg, _recv_pid) <= 0) {
      puts("gnrc_netdev: possibly lost interrupt.");
    }
  } else {
    size_t len;
    netdev_lora_rx_info_t packet_info;
    switch (event) {
    case NETDEV_EVENT_RX_STARTED:
      // puts("Data reception started");
      break;

    case NETDEV_EVENT_RX_COMPLETE:
      len = dev->driver->recv(dev, NULL, 0, 0);
      dev->driver->recv(dev, message, len, &packet_info);

      if (_message_callback) {
        _message_callback(len, message, packet_info.rssi, packet_info.snr,
                          sx127x_get_time_on_air((const sx127x_t *)dev, len));
      }
      break;

    case NETDEV_EVENT_TX_COMPLETE:
      sx127x_set_sleep(&sx127x);
      // puts("Transmission completed");
      break;

    case NETDEV_EVENT_CAD_DONE:
      break;

    case NETDEV_EVENT_TX_TIMEOUT:
      sx127x_set_sleep(&sx127x);
      break;

    default:
      // printf("Unexpected netdev event received: %d\n", event);
      break;
    }
  }
}

void *_recv_thread(void *arg) {
  (void)arg;

  static msg_t _msg_q[SX127X_LORA_MSG_QUEUE];

  msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE);

  while (1) {
    msg_t msg;
    msg_receive(&msg);
    if (msg.type == MSG_TYPE_ISR) {
      netdev_t *dev = msg.content.ptr;
      dev->driver->isr(dev);
    } else {
      // puts("Unexpected msg type");
    }
  }
}

int init_sx1272(int argc, char **argv) {
  (void)argc;
  (void)argv;
  sx127x.params = sx127x_params[0];
  netdev_t *netdev = &sx127x.netdev;

  netdev->driver = &sx127x_driver;
  netdev->event_callback = _event_cb;

  if (netdev->driver->init(netdev) < 0) {
    puts("Failed to initialize SX127x device, exiting");
    return 1;
  }

  _recv_pid =
      thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                    THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv_thread");

  if (_recv_pid <= KERNEL_PID_UNDEF) {
    puts("Creation of receiver thread failed");
    return 1;
  } else {
    puts("SX127x initialization successful");
  }

  return 0;
}
