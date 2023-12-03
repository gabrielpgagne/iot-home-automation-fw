/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>

#define SERVICE_DATA_LEN 9
#define SERVICE_UUID 0xfcd2 /* BTHome service UUID */
#define IDX_TEMPL 4         /* Index of lo byte of temp in service data*/
#define IDX_TEMPH 5         /* Index of hi byte of temp in service data*/

// Example
// https://github.com/aaron-mohtar-co/Lemon-IoT-LTE-nRF9160/tree/main/Examples/i2c_sensor
// #define OPT3001_DEV DT_NODELABEL(opt3001)
// #define SHTC_DEV DT_NODELABEL(shtcx)
#define DOOR DT_ALIAS(sw0)
#define CTRL DT_ALIAS(sw1)
#define LED0 DT_ALIAS(led0)
#define BUZZER DT_ALIAS(led1)

#define ADV_PARAMS                                                             \
  (BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME, BT_GAP_ADV_FAST_INT_MIN_2,          \
                   BT_GAP_ADV_FAST_INT_MAX_2, NULL))

// DeviceTree devices
const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER, gpios);
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0, gpios);
const struct gpio_dt_spec door_sw = GPIO_DT_SPEC_GET(DOOR, gpios);
const struct gpio_dt_spec ctrl_btn = GPIO_DT_SPEC_GET(CTRL, gpios);
// const struct device *light = DEVICE_DT_GET(OPT3001_DEV);
// const struct device *sht = DEVICE_DT_GET(SHTC_DEV);

static struct gpio_callback door_cb;
static struct gpio_callback ctrl_cb;

// TODO this code is disgusting, software debounce the buttons
// https://github.com/ubieda/zephyr_button_debouncing/tree/master
int ctrl_btn_state = 0;
int ctrl_btn_dt = 0;
// Beware this function assumes debounced button.
void ctrl_button_pressed(const struct device *dev, struct gpio_callback *cb,
                         uint32_t pins) {
  printk("Control button pressed at %" PRIu32 "\n", k_cycle_get_32());

  switch (ctrl_btn_state) {
  // Idle
  case 0:
    if (gpio_pin_get_dt(&ctrl_btn) == 1) {
      printk("Control button pressed at %" PRIu32 "\n", k_cycle_get_32());
      ctrl_btn_state = 1;
    }
    break;
  // Button released
  case 1:
    if (gpio_pin_get_dt(&ctrl_btn) == 0) {
      if (k_cyc_to_ms_ceil32(k_cycle_get_32()) - ctrl_btn_dt < 3000) {
        printk("Ctrl Btn released, going to quiet mode.\n");
        ctrl_btn_state = 2;
        // TODO Force buzzer off for 30s
      } else {
        printk("Ctrl Btn released, going to shutdown mode.\n");
        ctrl_btn_state = 3;
        // TODO force shutdown
      }
    }
    break;
  case 2:
  case 3:
    ctrl_btn_state = 0;
    break;
  }
  ctrl_btn_dt = k_cyc_to_ms_ceil32(k_cycle_get_32());
}

int door_switch_dt = 0;
void door_state_change(const struct device *dev, struct gpio_callback *cb,
                       uint32_t pins) {
  printk("Door state change at %" PRIu32 "\n", k_cycle_get_32());
  int door_state = gpio_pin_get_dt(&door_sw);
  if (door_state == 0) {
    door_switch_dt = k_cyc_to_ms_ceil32(k_cycle_get_32());
    gpio_pin_set_dt(&buzzer, 0);
  } else {
    int dt = k_cyc_to_ms_ceil32(k_cycle_get_32()) - door_switch_dt;
    if (dt > 1000) {
      gpio_pin_set_dt(&buzzer, 1);
    }
  }
}

static uint8_t service_data[SERVICE_DATA_LEN] = {
    BT_UUID_16_ENCODE(SERVICE_UUID),
    0x40,
    0x02, /* Temperature */
    0xc4, /* Low byte */
    0x00, /* High byte */
    0x03, /* Humidity */
    0xbf, /* 50.55%  low byte*/
    0x13, /* 50.55%  high byte*/
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    // BT_DATA(BT_DATA_SVC_DATA16, struct sensor_value temp, hum;
    //         service_data, ARRAY_SIZE(service_data))};
};

static int bt_ready() {

  printk("Bluetooth initialized\n");

  /* Start advertising */
  int err = bt_le_adv_start(ADV_PARAMS, ad, ARRAY_SIZE(ad), NULL, 0);
  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
  }
  return err;
}

int main(void) {
  int err;
  int temp = 0;

  // ----- Init Door Switch + IRQ -----
  err = gpio_pin_configure_dt(&door_sw, GPIO_INPUT);
  err = gpio_pin_interrupt_configure_dt(&door_sw, GPIO_INT_EDGE_BOTH);
  if (err != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", err,
           door_sw.port->name, door_sw.pin);
    return err;
  }
  gpio_init_callback(&door_cb, door_state_change, BIT(door_sw.pin));
  gpio_add_callback(door_sw.port, &door_cb);

  // ----- Init CTRL Button + IRQ -----
  err = gpio_pin_configure_dt(&ctrl_btn, GPIO_INPUT | GPIO_PULL_DOWN);
  err = gpio_pin_interrupt_configure_dt(&ctrl_btn, GPIO_INT_EDGE_BOTH);
  if (err != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", err,
           ctrl_btn.port->name, ctrl_btn.pin);
    return err;
  }
  gpio_init_callback(&ctrl_cb, ctrl_button_pressed, BIT(ctrl_btn.pin));
  gpio_add_callback(ctrl_btn.port, &ctrl_cb);

  // ----- Init Buzzer -----
  gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT);
  gpio_pin_set_dt(&buzzer, 1);
  k_msleep(1000);
  gpio_pin_set_dt(&buzzer, 0);

  // ----- Init LED -----
  gpio_pin_configure_dt(&led, GPIO_OUTPUT);

  // ----- SHT init -----
  // if (!device_is_ready(sht)) {
  //  printf("Device %s is not ready\n", sht->name);
  // return 0;
  //} else {
  //  printk("Found device %s. Reading sensor data\n", sht->name);
  //}

  while (1) {
    ;
  }
  // ----- Init BLE -----
  printk("%s\n", CONFIG_BT_DEVICE_NAME);
  err = bt_enable(NULL);
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return 0;
  }
  err = bt_ready();
  if (err) {
    printk("Bluetooth ready failed (err %d)\n", err);
    return 0;
  }

  // ----- Main loop -----
  for (;;) {
    /* Simulate temperature from 0C to 25C */
    service_data[IDX_TEMPH] = (temp * 100) >> 8;
    service_data[IDX_TEMPL] = (temp * 100) & 0xff;
    if (temp++ == 25) {
      temp = 0;
    }
    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
      printk("Failed to update advertising data (err %d)\n", err);
    }
    k_sleep(K_MSEC(BT_GAP_ADV_SLOW_INT_MIN));
  }
  return 0;
}
