/*
 * BTHome v2 advertiser for the basement temperature/humidity sensor.
 *
 * The sensor is a pure broadcaster as far as Home Assistant is concerned, but
 * advertising stays connectable so that MCUmgr OTA DFU keeps working.
 */

#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include <stdbool.h>
#include <stdint.h>

void ble_disable(void);
int ble_init(void);

/* Each setter only stages a value in the advertising payload; nothing is sent
 * over the air until bt_publish() is called. */
void bt_update_temp(float temp);
void bt_update_humidity(float hm);
void bt_update_battery(bool charging, uint8_t batt);
void bt_update_all(float temp, float hm, bool charging, uint8_t batt);

/* Increments the BTHome packet id and pushes the payload to the advertiser. */
int bt_publish(void);

#endif /* __BLUETOOTH_H__ */
