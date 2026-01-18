/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bluetooth.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#define SERVICE_UUID 0xfcd2 /* BTHome service UUID */
#define IDX_TEMPL 4         /* Index of lo byte of temp in service data*/
#define IDX_TEMPH 5         /* Index of hi byte of temp in service data*/
#define IDX_HUMDL 7         /* Index of lo byte of humidity in service data*/
#define IDX_HUMDH 8         /* Index of hi byte of humidity in service data*/
#define IDX_DOOR 10         /* Index of door state */
#define IDX_BATT 12         /* Index of state of charge */
#define IDX_CHARGING 14     /* Index of charging state */

static uint8_t service_data[] = {
    BT_UUID_16_ENCODE(SERVICE_UUID),
    0x40,
    0x02, /* Temperature */
    0xc4, /* Low byte */
    0x00, /* High byte */
    0x03, /* Humidity */
    0xbf, /* low byte */
    0x13, /* high byte */
    0x1A, /* Door alarm */
    0x00, /* 0 = closed, 1 = open */
    0x01, /* Battery% */
    80,   /* 50% */
    0x16, /* Battery charging */
    0x00, /* 0 = not charging, 1 = charging */
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data))};

static void connected(struct bt_conn* conn, uint8_t err) {
  if (err) {
    printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
    return;
  }

  printk("Connected\n");
}

static void disconnected(struct bt_conn* conn, uint8_t reason) {
  printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(int err) {
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return;
  }

  /* Start advertising */
  // MUST BE BT_LE_ADV_CONN to be able to DFU
  const struct bt_le_adv_param* adv_params = BT_LE_ADV_PARAM(
      BT_LE_ADV_CONN, BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL);

  err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
    return;
  }
}

void ble_disable(void) { bt_disable(); }

int ble_init(void) {
  int err;

  /* Initialize the Bluetooth Subsystem */
  err = bt_enable(bt_ready);
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return err;
  }

  return 0;
}

void bt_update_temp(float temp) {
  service_data[IDX_TEMPH] = (int)(temp * 100) >> 8;
  service_data[IDX_TEMPL] = (int)(temp * 100) & 0xff;
}

void bt_update_humidity(float hm) {
  service_data[IDX_HUMDH] = (int)(hm * 100) >> 8;
  service_data[IDX_HUMDL] = (int)(hm * 100) & 0xff;
}

void bt_update_door_state(bool open) { service_data[IDX_DOOR] = open ? 1 : 0; }

void bt_update_battery(bool charging, uint8_t batt) {
  service_data[IDX_BATT] = batt;
  service_data[IDX_CHARGING] = charging ? 1 : 0;
}

void bt_update_all(float temp, float hm, bool open) {
  bt_update_temp(temp);
  bt_update_humidity(hm);
  bt_update_door_state(open);
}

int bt_publish() {
  int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    printk("Failed to update advertising data (err %d)\n", err);
  }
  return err;
}