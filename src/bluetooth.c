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
#include <zephyr/sys/byteorder.h>

#define SERVICE_UUID 0xfcd2 /* BTHome service UUID */

/* BTHome v2 object ids, kept in ascending order inside the payload as the
 * specification recommends. */
#define BTHOME_ID_PACKET 0x00
#define BTHOME_ID_BATTERY 0x01
#define BTHOME_ID_TEMP 0x02
#define BTHOME_ID_HUMIDITY 0x03
#define BTHOME_ID_CHARGING 0x16

#define IDX_PACKET 4    /* Packet id counter */
#define IDX_BATT 6      /* State of charge, percent */
#define IDX_TEMP 8      /* Temperature, little endian sint16 */
#define IDX_HUMD 11     /* Humidity, little endian uint16 */
#define IDX_CHARGING 14 /* Charging state */

static uint8_t service_data[] = {
    BT_UUID_16_ENCODE(SERVICE_UUID),
    0x40, /* BTHome v2, unencrypted, regular updates */
    BTHOME_ID_PACKET,
    0x00, /* Counter, incremented on every publish */
    BTHOME_ID_BATTERY,
    100, /* Percent */
    BTHOME_ID_TEMP,
    0x00, /* Low byte, sint16, 0.01 degC */
    0x00, /* High byte */
    BTHOME_ID_HUMIDITY,
    0x00, /* Low byte, uint16, 0.01 %RH */
    0x00, /* High byte */
    BTHOME_ID_CHARGING,
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
  // MUST BE BT_LE_ADV_OPT_CONNECTABLE to be able to DFU

  const struct bt_le_adv_param* adv_params =
      BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_SLOW_INT_MIN,
                      BT_GAP_ADV_SLOW_INT_MAX, NULL);

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
  /* BTHome temperature is a sint16 in 0.01 degC steps. */
  int16_t raw = (int16_t)(temp * 100.0f);
  sys_put_le16((uint16_t)raw, &service_data[IDX_TEMP]);
}

void bt_update_humidity(float hm) {
  /* BTHome humidity is a uint16 in 0.01 %RH steps. */
  if (hm < 0.0f) {
    hm = 0.0f;
  } else if (hm > 100.0f) {
    hm = 100.0f;
  }
  sys_put_le16((uint16_t)(hm * 100.0f), &service_data[IDX_HUMD]);
}

void bt_update_battery(bool charging, uint8_t batt) {
  service_data[IDX_BATT] = batt;
  service_data[IDX_CHARGING] = charging ? 1 : 0;
}

void bt_update_all(float temp, float hm, bool charging, uint8_t batt) {
  bt_update_temp(temp);
  bt_update_humidity(hm);
  bt_update_battery(charging, batt);
}

int bt_publish() {
  /* Home Assistant uses the packet id to drop duplicated adverts and to spot
   * updates it missed, so it has to change on every new reading. */
  service_data[IDX_PACKET]++;

  int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    printk("Failed to update advertising data (err %d)\n", err);
  }
  return err;
}
