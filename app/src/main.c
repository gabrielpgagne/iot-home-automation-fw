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

#include <button.h>
#include <blinker.h>

#include "fsm.h"

/*
 ***************************************************************************
 * Device tree definitions.
 ***************************************************************************
 */

#define SHTC_DEV DT_NODELABEL(shtcx)
#define DOOR DT_ALIAS(sw0)
#define CTRL DT_ALIAS(sw1)
#define STATUSLED DT_ALIAS(statusled)
#define BUZZER DT_ALIAS(buzzer)

// DeviceTree devices
const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER, gpios);
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(STATUSLED, gpios);
const struct gpio_dt_spec door_sw = GPIO_DT_SPEC_GET(DOOR, gpios);
const struct gpio_dt_spec ctrl_sw = GPIO_DT_SPEC_GET(CTRL, gpios);
//const struct device *light = DEVICE_DT_GET(OPT3001_DEV);
const struct device *sht = DEVICE_DT_GET(SHTC_DEV);


/*
 ***************************************************************************
 * Bluetooth BtHome setup.
 * ref: https://bthome.io/format/
 ****************************************************************************
 */
#define SERVICE_DATA_LEN 11 /* 11 bytes of service data */
#define SERVICE_UUID 0xfcd2 /* BTHome service UUID */
#define IDX_TEMPL 4         /* Index of lo byte of temp in service data*/
#define IDX_TEMPH 5         /* Index of hi byte of temp in service data*/
#define IDX_HUMDL 7         /* Index of lo byte of humidity in service data*/
#define IDX_HUMDH 8         /* Index of hi byte of humidity in service data*/
#define IDX_DOOR  10         /* Index of door state */

static uint8_t service_data[SERVICE_DATA_LEN] = {
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
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
   BT_DATA(BT_DATA_SVC_DATA16,
            service_data, ARRAY_SIZE(service_data)),
};

/*
 #define ADV_PARAMS                                                             \
   BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME, BT_GAP_ADV_FAST_INT_MIN_2,          \
                    BT_GAP_ADV_FAST_INT_MAX_2, NULL)
*/

#define ADV_PARAMS BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
          BT_GAP_ADV_SLOW_INT_MIN, \
          BT_GAP_ADV_SLOW_INT_MAX, NULL)


/*
 ***************************************************************************
 * Contexts for buttons and blinkers.
 ****************************************************************************
 */

struct button_context door_sw_context;	// keep it alive !
struct button_context ctrl_sw_context;	// keep it alive !
struct blinker_context userled_blinker_context;	// keep it alive !
struct blinker_context buzzer_blinker_context;	// keep it alive !
struct blinker_context usertimer_context;	// keep it alive !

/*
 ***************************************************************************
 buttons and blinkers callbacks.
 ****************************************************************************
 */
static void userled_blink_event_handler(enum blinker_evt evt, long)
{
    gpio_pin_set_dt(&led, evt==BLINKER_EVT_ON ? 1 : 0);
}

static void buzzer_event_handler(enum blinker_evt evt, long)
{
    gpio_pin_set_dt(&buzzer, evt==BLINKER_EVT_ON ? 1 : 0);
}

// We use a blinker as a one-shot timer
static void user_timer_event_handler(enum blinker_evt evt, long)
{
    printk("user_timer_event_handler: %d\n", (int)evt);
    if (evt==BLINKER_EVT_OFF)
    {
        FSM_Timer();
    }
}

static void ctrl_button_event_handler(enum button_evt evt, long)
{
    printk("Ctrl Button event: %d\n", (int)evt);
    switch (evt)
    {
        case BUTTON_EVT_PRESSED:
            FSM_UserPress();
            break;
        case BUTTON_EVT_RELEASED:
            FSM_UserRelease();
            break;
    }
}

static void door_button_event_handler(enum button_evt evt, long)
{
    printk("Door Button event: %d\n", (int)evt);
    switch (evt)
    {
        case BUTTON_EVT_PRESSED:
            FSM_DoorClose();
            break;
        case BUTTON_EVT_RELEASED:            
            FSM_DoorOpen();
            break;
    }
}

/*
 ***************************************************************************
 * Bluetooth functions.
 ****************************************************************************
 */
static void bt_ready(int err)
{
    printk("Bluetooth initialized\n");

    /* Start advertising */
    err = bt_le_adv_start(ADV_PARAMS, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
    }

    printk("Advertising started.\n");
}

/*
 ***************************************************************************
 * Main
 ****************************************************************************
 */
int main(void) {
    bool status_ok;

    printk("Hello !\n");
 
    // ----- Init Buzzer -----
    gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT);

    // ----- Init LED -----
    gpio_pin_configure_dt(&led, GPIO_OUTPUT);

    // ----- Confirm buzzer and led
    gpio_pin_set_dt(&led, 1);
    gpio_pin_set_dt(&buzzer, 1);
    k_msleep(100);
    gpio_pin_set_dt(&buzzer, 0);
    gpio_pin_set_dt(&led, 0);
    k_msleep(100);

    // ----- Init userlink blink -----
    status_ok = blinker_init(
                &userled_blinker_context,
                userled_blink_event_handler,
                0);
    
    status_ok = blinker_init(
                &buzzer_blinker_context,
                buzzer_event_handler,
                0);

    // ----- We use a blinker in a monostable fashion as a timer -----
    status_ok = blinker_init(
                &usertimer_context,
                user_timer_event_handler,
                0);

    // ----- Init button -----
    printk("bot1\n");
    status_ok = button_init(&door_sw_context,
                        &door_sw,
                        door_button_event_handler,
                        0,
                        0);

    if (!status_ok) {
        printk("Button 1 Init failed\n");
        return -1;
    }

    status_ok = button_init(&ctrl_sw_context,
                        &ctrl_sw,
                        ctrl_button_event_handler,
                        1,
                        GPIO_PULL_DOWN);

    if (!status_ok) {
        printk("Button 2 Init failed.\n");
        return -1;
    }

    // ----- SHT init -----
    if (!device_is_ready(sht)) {
        printk("Device %s is not ready\n", sht->name);

    } else {
        printk("Found device %s. Reading sensor data\n", sht->name);
    }
    
    // ----- BT init -----
    printk("CONFIG_BT_DEVICE_NAME: %s\n", CONFIG_BT_DEVICE_NAME);

   	int err = bt_enable(bt_ready);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);

        for (int i=0; i<5; ++i)
        {
            gpio_pin_set_dt(&buzzer, 1);
            k_msleep(100);
            gpio_pin_set_dt(&buzzer, 0);
            k_msleep(1000);
        }

        return 0;
    }

   	/* Give time to bt_ready sequence */
	k_sleep(K_SECONDS(6));

    printk("BLE disable\n");
    err = bt_disable();
    if (err) {
        printk("Bluetooth disable failed (err %d)\n", err);
    }
    
    printk("Bluetooth init done.\n");

    // ----- State machine init -----
    FSM_Init();

    // init buttons
    button_reset_state(&door_sw_context);
    button_reset_state(&ctrl_sw_context);

    // ----- Main loop -----
    int temp_sensor_count = 0;
    for (;;)
    {
        /* Initialize the Bluetooth Subsystem */
        err = bt_enable(bt_ready);
        if (err) {
            printk("Bluetooth reinit failed (err %d)\n", err);
        }

        /* Give time to bt_ready sequence */
        k_sleep(K_SECONDS(1));

        bool state_changed = false;

        /* Get temp & humidity */
        struct sensor_value temp, hum;
        ++temp_sensor_count;
        if (temp_sensor_count >= 5 && device_is_ready(sht))
        {
            temp_sensor_count = 0;

            int err = sensor_sample_fetch(sht);
            if (err == 0)
            {
                err = sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP,
                    &temp);
                if (err==0)
                {
                    err = sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY,
                        &hum);
                }
            }
            if (err != 0)
            {
                printf("SHT: failed: %d\n", err);
            }
            else
            {
                double ftemp = sensor_value_to_double(&temp);
                double fhumd = sensor_value_to_double(&hum);
                // printf("SHT: %.2f Cel ; %0.2f %%RH\n", ftemp, fhumd);

                int temp_h = (int)(ftemp * 100) >> 8;
                int temp_l = (int)(ftemp * 100) & 0xff;
                int hum_h = (int)(fhumd * 100) >> 8;
                int hum_l = (int)(fhumd * 100) & 0xff;

                if (service_data[IDX_TEMPH] != temp_h
                    || service_data[IDX_TEMPL] != temp_l
                    || service_data[IDX_HUMDH] != hum_h
                    || service_data[IDX_HUMDL] != hum_l)
                {
                    service_data[IDX_TEMPH] = temp_h;
                    service_data[IDX_TEMPL] = temp_l;

                    service_data[IDX_HUMDH] = hum_h;
                    service_data[IDX_HUMDL] = hum_l;
                    state_changed = true;
                }
            }
        }
        else
        {
            // No device present or not responding
            service_data[IDX_TEMPH] = 0;
            service_data[IDX_TEMPL] = 0;

            service_data[IDX_HUMDH] = 0;
            service_data[IDX_HUMDL] = 0;
        }

        /* Get door state */
        int door_open = door_sw_context.pressed == 0 ? 1 : 0;
        
        if (service_data[IDX_DOOR] != door_open)
        {
            service_data[IDX_DOOR] = door_open;
            state_changed = true;
        }
    
        /* Update advertising data */
        if (state_changed)
        {

            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
            if (err)
            {
                printk("Failed to update advertising data (err %d)\n", err);
                for (int i=0; i<20; ++i)
                {
                    gpio_pin_set_dt(&buzzer, 1);
                    k_msleep(100);
                    gpio_pin_set_dt(&buzzer, 0);
                    k_msleep(1000);
                }
            }
        }

        printk("BLE disable\n");
        err = bt_disable();
        if (err) {
            printk("Bluetooth disable failed (err %d)\n", err);
        }

        k_sleep(K_SECONDS(30));
        //k_sleep(K_MINUTES(5));
    }
    return 0;
}

/******** State machine event handlers ********/
// Transition function "prepareWatching"
void FSM_prepareWatching(void)
{
    // Set timer to 30 sec
	// blink rled
	// stop buzzer
	// gled off

    blinker_sequence0(&usertimer_context, 30000);
    blinker_start(&usertimer_context, false);

    blinker_sequence2(&userled_blinker_context, 100, 900);
    blinker_start(&userled_blinker_context, true);

    blinker_stop(&buzzer_blinker_context, false);
}

// Transition function "prepareQuiet"
void FSM_prepareQuiet(void)
{
    // stop buzzer
	// set timer to 30 mn
	// blink green led
	// rled off

    blinker_stop(&buzzer_blinker_context, false);

    blinker_sequence0(&usertimer_context, 30*60*1000);
    blinker_start(&usertimer_context, false);

    blinker_sequence4(&userled_blinker_context, 100, 100, 100, 1500);
    blinker_start(&userled_blinker_context, true);
}

void FSM_userRelease(void)
{
    // Simply disable the power shutdown timer.
//	powerOffTimer.stop();
}

// Transition function "prepareIdle"
void FSM_prepareIdle(void)
{
    // stop buzzer
	// stop timer
	// leds off
	// go to sleep

    blinker_stop(&buzzer_blinker_context, false);
    blinker_stop(&usertimer_context, false);
    blinker_stop(&userled_blinker_context, false);
}

// Transition function "prepareAlarm"
void FSM_prepareAlarm(void)
{
	// stop timer
	// start buzzer
	// gled off
	// rled on

    blinker_stop(&usertimer_context, false);

    blinker_sequence2(&userled_blinker_context, 500, 500);
    blinker_start(&userled_blinker_context, true);

    blinker_sequence2(&buzzer_blinker_context, 500, 500);
    blinker_start(&buzzer_blinker_context, true);
}
