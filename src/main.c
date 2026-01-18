#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/smf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "bluetooth.h"
#include "charger.h"
#include "redef.h"

#define PWR_BTN_MS 1000

// For LEDs, check config.overlay: only controllable if mode = "host"
#define NPM_RED_LED 0
#define NPM_BLUE_LED 1
#define NPM_GREEN_LED 2

// ===================== Devicetree nodes ======================

const struct device* pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));
const struct device* leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));
const struct device* regulators =
    DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));
const struct device* ldsw = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo2));
const struct device* charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));
const struct device* sht = DEVICE_DT_GET(DT_NODELABEL(shtcx));
struct gpio_dt_spec user_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(sw0), gpios);
struct gpio_dt_spec door_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(sw1), gpios);

volatile bool vbus_connected;
// ===================== FSM globals ======================

#define FSM_MEASUREMENT_TIME_IDLE K_MINUTES(10)
#define FSM_MEASUREMENT_TIME_ACTIVE K_SECONDS(3)
#define FSM_DOOR_OPEN_GUARD_PERIOD K_MINUTES(1)
#define SILENT_MODE_DURATION K_MINUTES(5)

/* List of events */
#define EVENT_DOOR_OPEN BIT(0)
#define EVENT_DOOR_CLOSE BIT(1)
#define EVENT_SILENT_BTN_PRESSED BIT(2)
#define EVENT_SILENT_EXPIRED BIT(3)
#define EVENT_BUZZER_EXPIRED BIT(4)

static const struct smf_state fsm_states[];
static struct gpio_callback user_btn_cb_data;
static struct gpio_callback door_switch_cb_data;

enum fsm_state { STATE_IDLE, STATE_DOOR_OPEN, STATE_SILENT };

/* User defined object */
struct s_object {
  /* This must be first */
  struct smf_ctx ctx;

  /* Events */
  struct k_event smf_event;
  int32_t events;

  /* Other state specific data add here */
} s_obj;

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
        regulator_parent_ship_mode(regulators);
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

  if (!device_is_ready(ldsw)) {
    printk("Load switch device not ready.\n");
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

// ===================== Sensor measurements timer ======================

void update_bthome_channels(struct k_work* work) {
  struct sensor_value temp, hum;
  led_on(leds, NPM_GREEN_LED);
  int err = sensor_sample_fetch(sht);
  if (err) {
    printk("Failed to fetch data from SHT sensor\n");
  } else {
    err = sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    err = sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
    double ftemp = sensor_value_to_double(&temp);
    double fhumd = sensor_value_to_double(&hum);
    int soc = charger_get_soc(charger);
    bool open = gpio_pin_get_dt(&door_btn);
    printf("SHT: %.2f Cel; %0.2f %%RH; Door %s \n", ftemp, fhumd,
           open ? "open" : "closed");
    bt_update_all(ftemp, fhumd, open);
    bt_update_battery(vbus_connected, soc);
    bt_publish();
  }
  led_off(leds, NPM_GREEN_LED);
}

K_WORK_DEFINE(measurement_work_item, update_bthome_channels);

void measurement_timer_expired_cb(struct k_timer* dummy) {
  k_work_submit(&measurement_work_item);
}

K_TIMER_DEFINE(measurement_timer, measurement_timer_expired_cb, NULL);

// ===================== Door switch callback ======================

void door_wait_timer_expired_cb(struct k_timer* dummy) {
  if (gpio_pin_get_dt(&door_btn)) {
    k_event_post(&s_obj.smf_event, EVENT_DOOR_OPEN);
  } else {
    k_event_post(&s_obj.smf_event, EVENT_DOOR_CLOSE);
  }
}

K_TIMER_DEFINE(door_open_wait_timer, door_wait_timer_expired_cb, NULL);

void door_state_changed(const struct device* dev, struct gpio_callback* cb,
                        uint32_t pins) {
  int state = gpio_pin_get_dt(&door_btn);
  printk("Door state changed to %d\n", state);
  if (state) {
    k_timer_start(&door_open_wait_timer, FSM_DOOR_OPEN_GUARD_PERIOD, K_FOREVER);
  } else {
    k_timer_start(&door_open_wait_timer, K_NO_WAIT, K_FOREVER);
  }
}

// ===================== Silent mode timer ======================

void silent_mode_button_pressed(const struct device* dev,
                                struct gpio_callback* cb, uint32_t pins) {
  printk("Silent mode button press %d\n", gpio_pin_get_dt(&user_btn));
  k_event_post(&s_obj.smf_event, EVENT_SILENT_BTN_PRESSED);
}

void silent_mode_timer_expired_cb(struct k_timer* dummy) {
  k_event_post(&s_obj.smf_event, EVENT_SILENT_EXPIRED);
}

K_TIMER_DEFINE(silent_mode_timer, silent_mode_timer_expired_cb, NULL);

// ===================== Buzzer timer ======================

void buzzer_timer_expired_cb(struct k_timer* dummy) {
  k_event_post(&s_obj.smf_event, EVENT_BUZZER_EXPIRED);
}

K_TIMER_DEFINE(buzzer_timer, buzzer_timer_expired_cb, NULL);

// ===================== State machine definition ======================

void state_idle_entry(void* o) {
  printk("Enter idle state\n");
  k_timer_start(&measurement_timer, FSM_MEASUREMENT_TIME_IDLE,
                FSM_MEASUREMENT_TIME_IDLE);
  k_timer_stop(&buzzer_timer);
  k_timer_stop(&silent_mode_timer);
}

void state_idle_run(void* o) {
  struct s_object* s = (struct s_object*)o;

  if (s->events & EVENT_SILENT_BTN_PRESSED) {
    smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_SILENT]);
  } else if (s->events & EVENT_DOOR_OPEN) {
    smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_DOOR_OPEN]);
  }
}

void state_idle_exit(void* o) {
  printk("Exit idle state\n");
  k_timer_start(&measurement_timer, FSM_MEASUREMENT_TIME_ACTIVE,
                FSM_MEASUREMENT_TIME_ACTIVE);
}

void state_door_open_entry(void* o) {
  printk("Enter Door open state\n");
  k_timer_start(&buzzer_timer, K_NO_WAIT, K_FOREVER);
}

void state_door_open_run(void* o) {
  struct s_object* s = (struct s_object*)o;

  if (s->events & EVENT_SILENT_BTN_PRESSED) {
    smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_SILENT]);
  } else if (s->events & EVENT_DOOR_CLOSE) {
    smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_IDLE]);
  } else if (s->events & EVENT_BUZZER_EXPIRED) {
    if (!regulator_is_enabled(ldsw)) {
      // int ret = regulator_enable(ldsw);
      // if (ret) {
      //   printk("Failed to enable load switch\n");
      // }
      k_timer_start(&buzzer_timer, K_MSEC(200), K_FOREVER);
    } else {
      regulator_disable(ldsw);
      k_timer_start(&buzzer_timer, K_MSEC(800), K_FOREVER);
    }
  }
}

void state_door_open_exit(void* o) {
  printk("Exit Door open state\n");
  k_timer_stop(&buzzer_timer);
  if (regulator_is_enabled(ldsw)) {
    regulator_disable(ldsw);
  }
}

void state_silent_entry(void* o) {
  printk("Enter silent mode state\n");
  k_timer_start(&silent_mode_timer, SILENT_MODE_DURATION, K_FOREVER);
}

void state_silent_run(void* o) {
  struct s_object* s = (struct s_object*)o;

  if (s->events & EVENT_SILENT_EXPIRED) {
    if (gpio_pin_get_dt(&door_btn)) {
      smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_DOOR_OPEN]);
    } else {
      smf_set_state(SMF_CTX(&s_obj), &fsm_states[STATE_IDLE]);
    }
  }
}

void state_silent_exit(void* o) {
  printk("Exit silent mode state\n");
  k_timer_stop(&silent_mode_timer);
}

static const struct smf_state fsm_states[] = {
    [STATE_IDLE] = SMF_CREATE_STATE(state_idle_entry, state_idle_run,
                                    state_idle_exit, NULL, NULL),
    [STATE_DOOR_OPEN] =
        SMF_CREATE_STATE(state_door_open_entry, state_door_open_run,
                         state_door_open_exit, NULL, NULL),
    [STATE_SILENT] = SMF_CREATE_STATE(state_silent_entry, state_silent_run,
                                      state_silent_exit, NULL, NULL),
};

// ===================== Main code ======================

int main(void) {
  int ret;

  printk("Initializing...\n");

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

  gpio_init_callback(&user_btn_cb_data, silent_mode_button_pressed,
                     BIT(user_btn.pin));
  gpio_add_callback(user_btn.port, &user_btn_cb_data);

  // ========= Door switch =========
  ret = gpio_pin_configure_dt(&door_btn, GPIO_INPUT | GPIO_PULL_UP);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           door_btn.port->name, door_btn.pin);
    return 0;
  }

  ret = gpio_pin_interrupt_configure_dt(&door_btn, GPIO_INT_EDGE_BOTH);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           door_btn.port->name, door_btn.pin);
    return 0;
  }

  gpio_init_callback(&door_switch_cb_data, door_state_changed,
                     BIT(door_btn.pin));
  gpio_add_callback(door_btn.port, &door_switch_cb_data);

  // ========= State machine =========

  for (int i = 0; i < 3; i++) {
    led_on(leds, NPM_GREEN_LED);
    k_msleep(250);
    led_off(leds, NPM_GREEN_LED);
    k_msleep(250);
  }

  k_event_init(&s_obj.smf_event);
  smf_set_initial(SMF_CTX(&s_obj), &fsm_states[STATE_IDLE]);

  /* Run the state machine */
  while (1) {
    /* Block until an event is detected */
    s_obj.events = k_event_wait(&s_obj.smf_event, 0xffff, true, K_FOREVER);

    /* State machine terminates if a non-zero value is returned */
    ret = smf_run_state(SMF_CTX(&s_obj));
    if (ret) {
      break;
    }
  }
}
