/*
 * Basement temperature and humidity sensor.
 *
 * Reads the SHTC3 every MEASUREMENT_INTERVAL and republishes the result as a
 * BTHome v2 advertisement for Home Assistant. Advertising stays connectable so
 * that OTA DFU remains available at any time.
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "bluetooth.h"
#include "charger.h"
#include "redef.h"

#define PWR_BTN_MS 1000

/* How often the SHTC3 is read and the advertisement refreshed. A basement does
 * not move fast, so this is mostly a battery-life knob. */
#define MEASUREMENT_INTERVAL K_MINUTES(5)

// For LEDs, check config.overlay: only controllable if mode = "host"
#define NPM_RED_LED 0
#define NPM_BLUE_LED 1
#define NPM_GREEN_LED 2

// ===================== Devicetree nodes ======================

const struct device* pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));
const struct device* leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));
const struct device* regulators =
    DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));
const struct device* charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));
const struct device* sht = DEVICE_DT_GET(DT_NODELABEL(shtcx));
struct gpio_dt_spec user_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(sw0), gpios);

volatile bool vbus_connected;

/* Given by the user button to force a reading without waiting out the
 * interval, which is handy when checking the sensor against a reference. */
static K_SEM_DEFINE(measure_now, 0, 1);
static struct gpio_callback user_btn_cb_data;

// ===================== Charger configuration ======================

void npm_event_cb(const struct device* dev, struct gpio_callback* cb,
                  uint32_t pins) {
  static int press_t;

  if (pins & BIT(NPM1300_EVENT_SHIPHOLD_PRESS)) {
    printk("Power button pressed\n");
    press_t = k_uptime_get();
  }

  if (pins & BIT(NPM1300_EVENT_SHIPHOLD_RELEASE)) {
    press_t = k_uptime_get() - press_t;

    if (press_t > PWR_BTN_MS) {
      if (vbus_connected) {
        printk("Ship mode entry not possible with USB connected\n");
      } else {
        // regulator_parent_ship_mode(regulators);
      }
      printk("Short press\n");
    }
  }

  if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
    printk("Vbus connected\n");
    led_on(leds, NPM_RED_LED);
    vbus_connected = true;
    int ret = usb_enable(NULL);
    if (ret != 0) {
      printk("Failed to enable USB\n");
    }
  }

  if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
    printk("Vbus removed\n");
    led_off(leds, NPM_RED_LED);
    vbus_connected = false;
    usb_disable();
  }
}

bool configure_events(void) {
  if (!device_is_ready(pmic)) {
    printk("Pmic device not ready.\n");
    return false;
  }

  if (!device_is_ready(regulators)) {
    printk("Regulator device not ready.\n");
    return false;
  }

  if (!device_is_ready(charger)) {
    printk("Charger device not ready.\n");
    return false;
  }

  const struct npm1300_charger_config* const config = charger->config;

  int ret = mfd_npm1300_reg_write(config->mfd, 0x03, 0x06, 2);
  if (ret != 0) {
    printk("Failed to disable charger thermal monitoring\n");
  }

  static struct gpio_callback event_cb;

  gpio_init_callback(
      &event_cb, npm_event_cb,
      BIT(NPM1300_EVENT_SHIPHOLD_PRESS) | BIT(NPM1300_EVENT_SHIPHOLD_RELEASE) |
          BIT(NPM1300_EVENT_VBUS_DETECTED) | BIT(NPM1300_EVENT_VBUS_REMOVED));

  mfd_npm1300_add_callback(pmic, &event_cb);

  /* Initialise vbus detection status */
  struct sensor_value val;
  ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH,
                        &val);

  if (ret < 0) {
    return false;
  }

  vbus_connected = (val.val1 != 0) || (val.val2 != 0);
  printk("Vbus connected: %d\n", vbus_connected);

  if (vbus_connected) {
    led_on(leds, NPM_RED_LED);
    usb_enable(NULL);
  }

  return true;
}

// ===================== Reset diagnostics ======================

/* nPM1300 ERRLOG block, from the nPM1300 product specification. RSTCAUSE
 * survives the PMIC's own power cycle and says why it restarted the rails. */
#define NPM1300_ERRLOG_BASE 0x0EU
#define NPM1300_ERRLOG_TASKCLR 0x00U
#define NPM1300_ERRLOG_RSTCAUSE 0x03U

static void print_reset_cause(void) {
  uint32_t cause = 0;

  /* The nRF only sees its own side: a PMIC power cycle shows up here as a
   * power-on or brownout reset, hence the PMIC log below. */
  if (hwinfo_get_reset_cause(&cause) == 0) {
    printk("nRF reset cause 0x%08x:%s%s%s%s%s%s\n", cause,
           (cause & RESET_PIN) ? " pin" : "",
           (cause & RESET_SOFTWARE) ? " software" : "",
           (cause & RESET_BROWNOUT) ? " brownout" : "",
           (cause & RESET_POR) ? " power-on" : "",
           (cause & RESET_WATCHDOG) ? " watchdog" : "",
           (cause & RESET_CPU_LOCKUP) ? " lockup" : "");
    hwinfo_clear_reset_cause();
  }

  uint8_t rst;
  if (mfd_npm1300_reg_read(pmic, NPM1300_ERRLOG_BASE, NPM1300_ERRLOG_RSTCAUSE,
                           &rst) == 0) {
    printk("nPM1300 reset cause 0x%02x:%s%s%s%s%s%s%s\n", rst,
           (rst & BIT(0)) ? " ship-exit" : "",
           (rst & BIT(1)) ? " boot-monitor" : "",
           (rst & BIT(2)) ? " watchdog" : "",
           (rst & BIT(3)) ? " long-press" : "",
           (rst & BIT(4)) ? " thermal" : "",
           (rst & BIT(5)) ? " vsys-low" : "",
           (rst & BIT(6)) ? " software" : "");
    mfd_npm1300_reg_write(pmic, NPM1300_ERRLOG_BASE, NPM1300_ERRLOG_TASKCLR,
                          1U);
  } else {
    printk("nPM1300 reset cause unreadable\n");
  }
}

// ===================== Measurement ======================

static void user_btn_pressed(const struct device* dev,
                             struct gpio_callback* cb, uint32_t pins) {
  k_sem_give(&measure_now);
}

static bool measure_and_publish(void) {
  struct sensor_value temp, hum;
  bool ok = false;

  /* The activity LED is only worth its current draw while on USB; on battery
   * it would be a few mA for nobody to see in a dark basement. */
  if (vbus_connected) {
    led_on(leds, NPM_GREEN_LED);
  }

  int err = sensor_sample_fetch(sht);
  if (err) {
    printk("Failed to fetch data from SHT sensor (%d)\n", err);
  } else {
    err = sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (err) {
      printk("Failed to read temperature channel (%d)\n", err);
    }
    err = sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
    if (err) {
      printk("Failed to read humidity channel (%d)\n", err);
    }

    double ftemp = sensor_value_to_double(&temp);
    double fhumd = sensor_value_to_double(&hum);
    int soc = charger_get_soc(charger);
    if (soc < 0) {
      soc = 0;
    }

    printf("SHT: %.2f Cel; %.2f %%RH; SoC %d %%\n", ftemp, fhumd, soc);

    bt_update_all(ftemp, fhumd, vbus_connected, (uint8_t)soc);
    ok = (bt_publish() == 0);
  }

  if (vbus_connected) {
    led_off(leds, NPM_GREEN_LED);
  }

  return ok;
}

// ===================== Main code ======================

int main(void) {
  int ret;

  printk("Initializing...\n");
  print_reset_cause();

  ret = ble_init();
  if (ret) {
    printk("Failed to initialize BLE (%d)\n", ret);
    return 0;
  }

  if (!device_is_ready(leds)) {
    printk("Error: led device is not ready\n");
    return 0;
  }

  // FOR CHARGING:
  // https://devzone.nordicsemi.com/f/nordic-q-a/105379/npm1300-is-not-charging---is-ntc-to-gnd-the-reason
  if (!configure_events()) {
    printk("Error: could not configure PMIC\n");
    return 0;
  }

  if (!gpio_is_ready_dt(&user_btn)) {
    printk("Error: button device %s is not ready\n", user_btn.port->name);
    return 0;
  }

  if (!device_is_ready(sht)) {
    printk("Device %s is not ready\n", sht->name);
    return 0;
  }

  // ========= User Button =========
  ret = gpio_pin_configure_dt(&user_btn, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           user_btn.port->name, user_btn.pin);
    return 0;
  }

  ret = gpio_pin_interrupt_configure_dt(&user_btn, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           user_btn.port->name, user_btn.pin);
    return 0;
  }

  gpio_init_callback(&user_btn_cb_data, user_btn_pressed, BIT(user_btn.pin));
  gpio_add_callback(user_btn.port, &user_btn_cb_data);

  for (int i = 0; i < 3; i++) {
    led_on(leds, NPM_GREEN_LED);
    k_msleep(250);
    led_off(leds, NPM_GREEN_LED);
    k_msleep(250);
  }

  /* Publish once straight away so Home Assistant does not have to wait a full
   * interval to discover the device. */
  bool first_reading_ok = measure_and_publish();

  /* An image installed over DFU boots in the "test" state, and MCUboot swaps
   * the previous one back on the next reset unless it is confirmed. Confirm
   * only after a reading has actually gone out over the air, so a build that
   * cannot read the sensor or advertise still rolls itself back. */
  if (first_reading_ok && !boot_is_img_confirmed()) {
    ret = boot_write_img_confirmed();
    if (ret) {
      printk("Failed to confirm image (%d)\n", ret);
    } else {
      printk("Image confirmed, rollback disarmed\n");
    }
  }

  while (1) {
    /* Wakes early on a button press, otherwise once per interval. Either way
     * the next thing to do is take a reading. */
    k_sem_take(&measure_now, MEASUREMENT_INTERVAL);
    measure_and_publish();
  }
}
