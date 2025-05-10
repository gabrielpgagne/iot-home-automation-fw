/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "charger.h"

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/* nPM1300 CHARGER.BCHGCHARGESTATUS.CONSTANTCURRENT register bitmask */
#define NPM1300_CHG_STATUS_CC_MASK BIT(3)

static int read_sensors(const struct device *charger, float *voltage,
                        float *current, int32_t *chg_status) {
  struct sensor_value value;
  int ret;

  ret = sensor_sample_fetch(charger);
  if (ret < 0) {
    return ret;
  }

  sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
  *voltage = sensor_value_to_float(&value);

  //   sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &value);
  //   *temp = (float)value.val1 + ((float)value.val2 / 1000000);

  sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
  *current = sensor_value_to_float(&value);

  sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_STATUS, &value);
  *chg_status = value.val1;

  return 0;
}

int charger_get_soc(const struct device *charger) {
  float voltage = 0;
  float current = 0;
  //   float temp;
  float soc = 0;
  int32_t chg_status;
  bool cc_charging;
  int ret;

  ret = read_sensors(charger, &voltage, &current, &chg_status);
  if (ret < 0) {
    printk("Error: Could not read from charger device\n");
    return ret;
  }

  cc_charging = (chg_status & NPM1300_CHG_STATUS_CC_MASK) != 0;

  soc = ((double)voltage - 3.0) * 100 / (4.2 - 3.0);
  if (soc < 0) {
    soc = 0;
  } else if (soc > 100) {
    soc = 100;
  }

  printk("V: %.3f, I: %.3f, ", (double)voltage, (double)current);
  printk("SoC: %.2f\n", (double)soc);

  return soc;
}
