/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/gap.h"
#include "zephyr/bluetooth/uuid.h"
#include <stddef.h>
#include <sys/errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

// https://www.bluetooth.com/specifications/an/
// First 2 bytes = manufacturer, second is custom data
static uint8_t svc_data[20] = {0};

static const struct bt_data ad[] = {
    BT_DATA(BT_DATA_NAME_SHORTENED, "TPG", 3),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, svc_data, sizeof(svc_data))};

int main(void) {
  int err;

  printk("Starting Broadcaster\n");

  /* Initialize the Bluetooth Subsystem */
  err = bt_enable(NULL);
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return 0;
  }

  printk("Bluetooth initialized\n");

  // https://decoder.theengs.io/participate/adding-decoders.html
  // https://github.com/theengs/decoder/blob/development/src/devices/SHT4X_json.h
  // TLDR: check "condition" -> respect it and it'll work.
  // TODO find how to use BLE ADV Data Extension
  svc_data[0] = 0xd5;
  svc_data[1] = 0x06;
  svc_data[2] = 0x00;
  svc_data[3] = 0x06;

  do {
    k_msleep(1000);

    /* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
      int r = -EIO;
      printk("Advertising failed to start (err %d)\n", err);
      return 0;
    }

    k_msleep(1000);

    err = bt_le_adv_stop();
    if (err) {
      printk("Advertising failed to stop (err %d)\n", err);
      return 0;
    }

  } while (1);
  return 0;
}
